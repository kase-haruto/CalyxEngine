#include "PlayerDodgeMotion.h"
/* game */
#include <Game/3dObject/Actor/Player/Player.h>
#include <Game/3dObject/Actor/Player/Dodge/PlayerDodge.h>

// engine
#include <Engine/PostProcess/Manager/PostEffectManager.h>
#include <Engine/PostProcess/Blur/RadialBlur/RadialBlur.h>
#include <Engine/PostProcess/ChromaticAberration/ChromaticAberrationEffect.h>

/* util */
#include <Engine/Foundation/Utility/Func/CxUtils.h>

namespace{

	// Pull(引き)・Hold(溜め)・Return(戻り) の時間配分（g=0..1）
	constexpr float kPullBias = 0.72f; // 引きに割く比率（“ゆっくり引っ張る”体感）
	constexpr float kHoldRatio = 0.12f; // 頂点の溜め

	// 位置サーボ（位置誤差→一部だけ追従）
	constexpr float kServoHzPull = 6.0f;   // 引き／溜め中の追従周波数（大きいほどキビキビ）
	constexpr float kServoHzReturn = 14.0f;  // 戻りは素早く

	inline float ServoAlpha(float hz, float dt){
		return (dt <= 0.f) ? 0.f : (1.0f - std::exp(-hz * dt));
	}

	// 傾き・沈み
	constexpr float kPoseRollMaxScale = 0.22f;
	constexpr float kPosePitchMaxScale = 0.20f;
	constexpr float kPitchWeight = 0.6f;

	// 沈みの目標値と追従速度
	constexpr float kSinkStartup = -0.12f;
	constexpr float kSinkIFrame = -0.06f;
	constexpr float kSinkRecover = 0.00f;
	constexpr float kSinkHzStartup = 8.0f;
	constexpr float kSinkHzIFrame = 6.0f;
	constexpr float kSinkHzRecover = 10.0f;

	constexpr bool kMoveByTakesVelocity = true;

	// 1秒間無敵
	constexpr float kPerfectIFrameSec = 1.0f;

	inline float Saturate(float x){ return std::clamp(x, 0.0f, 1.0f); }
} // namespace

PlayerDodgeMotion::PlayerDodgeMotion()
	: owner_(nullptr)
	, dodge_(nullptr)
	, appliedOffset_(Vector3::Zero())
	, sinkCurrent_(0.0f)
	, additiveRoll_(0.0f)
	, additivePitch_(0.0f)
	, leanLerp_(0.0f)
	, spinQ_(Quaternion::MakeIdentity())
	, baseRot_(Quaternion::MakeIdentity())
{}
PlayerDodgeMotion::~PlayerDodgeMotion() = default;

void PlayerDodgeMotion::Initialize(Player* owner, PlayerDodge* dodge){
	owner_ = owner;
	dodge_ = dodge;

	// コールバック接続
	dodge_->SetOnDodgeStart([this]{ OnDodgeStart(); });
	dodge_->SetOnDodgeEnd([this]{ OnDodgeEnd();   });
	dodge_->SetOnPerfectDodge([this]{ OnPerfect();  });
}

void PlayerDodgeMotion::OnDodgeStart(){
	additiveRoll_ = additivePitch_ = 0.0f;
	leanLerp_ = 0.0f;

	// 姿勢の基準をキャプチャ
	baseRot_ = owner_->GetWorldTransform().rotation;
	spinQ_ = Quaternion::MakeIdentity();

	// 位置サーボ初期化
	appliedOffset_ = Vector3::Zero();

	// 沈みリセット（絶対値管理）
	sinkCurrent_ = 0.0f;
}

void PlayerDodgeMotion::OnDodgeEnd(){
	// --- 位置の最終スナップ---
	if (appliedOffset_.LengthSquared() > 1e-10f){
		owner_->GetWorldTransform().translation += (-appliedOffset_);
		appliedOffset_ = Vector3::Zero();
	}

	// --- 回転の最終スナップ
	owner_->GetWorldTransform().rotation = baseRot_;
	spinQ_ = Quaternion::MakeIdentity();

	additiveRoll_ = additivePitch_ = 0.0f;
	leanLerp_ = 0.0f;
	sinkCurrent_ = 0.0f;
}

void PlayerDodgeMotion::OnPerfect(){
	if (auto* rb = static_cast< RadialBlurEffect* >(
		PostEffectManager::Get()->GetPass("RadialBlur"))){
		PostEffectManager::Get()->TweenFloat(
			"RadialBlur",
			[rb]{ return rb->GetWidth(); },
			[rb] (float v){ rb->SetWidth(v); },
			std::nullopt, 0.05f, 0.10f, Cx::Ease::EaseType::EaseOutExpo, false,
			[rb]{
				PostEffectManager::Get()->TweenFloat(
					"RadialBlur",
					[rb]{ return rb->GetWidth(); },
					[rb] (float v){ rb->SetWidth(v); },
					std::nullopt, 0.0f, 0.45f, Cx::Ease::EaseType::EaseOutSine, true
				);
			}
		);
	}

	if (auto* ca = static_cast< ChromaticAberrationEffect* >(
		PostEffectManager::Get()->GetPass("ChromaticAberration"))){
		PostEffectManager::Get()->TweenFloat(
			"ChromaticAberration",
			[ca]{ return ca->GetIntensity(); },       // ← getter
			[ca] (float v){ ca->SetIntensity(v); },    // ← setter
			std::nullopt, 0.2f, 0.08f,
			Cx::Ease::EaseType::EaseOutExpo, false,
			[ca]{
				PostEffectManager::Get()->TweenFloat(
					"ChromaticAberration",
					[ca]{ return ca->GetIntensity(); },
					[ca] (float v){ ca->SetIntensity(v); },
					std::nullopt, 0.0f, 0.2f,
					Cx::Ease::EaseType::EaseOutSine, true
				);
			}
		);
	}
}

void PlayerDodgeMotion::Update(float dt){
	if (!owner_ || !dodge_) return;
	ApplySpinAndCurve(dt);   // 位置（Pull/Hold/Return）＋回転（絶対）
	ApplyProceduralPose(dt); // 姿勢＆沈み目標更新 → 最終姿勢合成
}

void PlayerDodgeMotion::ApplySpinAndCurve(float dt){
	if (dt <= 0.0f) return;

	const auto  st = dodge_->GetState();
	const auto& cfg = dodge_->Cfg();

	const float total = cfg.startup + cfg.duration + cfg.recovery;

	// g: 0..1（回避中）／-1（回避外）
	float g = -1.0f;
	if (total > 1e-6f){
		if (st == DodgeState::Startup)   g = (0.0f + dodge_->GetStateTime()) / total;
		else if (st == DodgeState::IFrame)    g = (cfg.startup + dodge_->GetStateTime()) / total;
		else if (st == DodgeState::Recovery)  g = (cfg.startup + cfg.duration + dodge_->GetStateTime()) / total;
		if (g >= 0.0f) g = Saturate(g);
	}

	/* ---------- 回転：絶対駆動（必ず回り切る） ---------- */
	{
		const float spinProgress = (g >= 0.0f) ? CalyxEase::EaseInCubic(g) : 0.0f; // 前半ゆっくり
		const float spinRad = (std::numbers::pi_v<float> *2.0f) * (cfg.spinTurns * spinProgress);
		spinQ_ = Quaternion::MakeRotateY(spinRad); // g から一意
	}

	/* ---------- 望ましい相対“位置”オフセット ---------- */
	Vector3 desiredCurve {0,0,0};
	float servoHz = kServoHzPull;

	if (g >= 0.0f){
		Vector3 fwd = dodge_->GetDodgeDir();
		if (fwd.LengthSquared() < 1e-6f) fwd = {0,0,1};
		fwd = fwd.Normalize();

		const float pullEnd = kPullBias * (1.0f - kHoldRatio);
		const float holdEnd = pullEnd + kHoldRatio;
		const float backAmp = cfg.distance * cfg.backwardScale;

		float back = 0.0f;
		if (g <= pullEnd){
			const float u = Saturate(g / pullEnd);
			back = backAmp * CalyxEase::EaseInCubic(u);           // ゆっくり引っ張る
			servoHz = kServoHzPull;
		} else if (g <= holdEnd){
			back = backAmp;                                       // 溜め
			servoHz = kServoHzPull;
		} else{
			const float v = Saturate((g - holdEnd) / (1.0f - holdEnd));
			back = backAmp * (1.0f - CalyxEase::EaseOutCubic(v)); // 素早く戻る
			servoHz = kServoHzReturn;
		}
		desiredCurve = -fwd * back;
	} else{
		desiredCurve = Vector3::Zero(); // 回避期間外は 0 に収束
		servoHz = kServoHzReturn;
	}

	// 沈み（絶対量）をYに合成
	const Vector3 desiredOffset = desiredCurve + Vector3(0, sinkCurrent_, 0);

	/* ---------- サーボ：誤差の一部だけ適用 ---------- */
	Vector3 delta = desiredOffset - appliedOffset_;
	const float alpha = ServoAlpha(servoHz, dt);
	const Vector3 step = delta * alpha; // 今フレーム“位置”として動かしたい分

	if constexpr (kMoveByTakesVelocity){
		owner_->AddMoveRequest(step);
	} else{
		owner_->GetWorldTransform().translation += step;
	}
	appliedOffset_ += step;
}

void PlayerDodgeMotion::ApplyProceduralPose(float dt){
	const auto  st = dodge_->GetState();
	const auto& cfg = dodge_->Cfg();

	// 入力方向
	Vector3 d = dodge_->GetDodgeDir();
	if (d.LengthSquared() > 1e-6f) d = d.Normalize(); else d = Vector3::Zero();

	const float maxRoll = kPoseRollMaxScale * (-d.x);
	const float maxPitch = kPosePitchMaxScale * (-d.y);

	if (st == DodgeState::Startup){
		float t = (cfg.startup > 0) ? Saturate(dodge_->GetStateTime() / cfg.startup) : 1.0f;
		t = CalyxEase::EaseOutCubic(t);
		leanLerp_ = Cx::Math::Lerp(leanLerp_, t, 0.5f);
	} else if (st == DodgeState::IFrame){
		leanLerp_ = Cx::Math::Lerp(leanLerp_, 1.0f, 0.35f);
	} else if (st == DodgeState::Recovery){
		float t = (cfg.recovery > 0) ? Saturate(dodge_->GetStateTime() / cfg.recovery) : 1.0f;
		t = CalyxEase::EaseInCubic(t);
		leanLerp_ = Cx::Math::Lerp(leanLerp_, 0.0f, 0.35f);
	} else{
		leanLerp_ = Cx::Math::Lerp(leanLerp_, 0.0f, 0.25f);
	}

	additiveRoll_ = maxRoll * leanLerp_;
	additivePitch_ = maxPitch * kPitchWeight * leanLerp_;

	const Quaternion poseQ =
		Quaternion::Multiply(
		Quaternion::MakeRotateZ(additiveRoll_),
		Quaternion::MakeRotateX(additivePitch_)
		);

	// 沈み（絶対量）の目標追従
	float sinkTarget = kSinkRecover;
	float sinkHz = kSinkHzRecover;
	if (st == DodgeState::Startup){ sinkTarget = kSinkStartup; sinkHz = kSinkHzStartup; } else if (st == DodgeState::IFrame){ sinkTarget = kSinkIFrame;  sinkHz = kSinkHzIFrame; } else if (st == DodgeState::Recovery){ sinkTarget = kSinkRecover; sinkHz = kSinkHzRecover; }
	const float a = ServoAlpha(sinkHz, dt);
	sinkCurrent_ = Cx::Math::Lerp(sinkCurrent_, sinkTarget, a);

	// 最終姿勢＝開始姿勢 × 絶対スピン × 追加姿勢（Slerpしない）
	if (IsDodging(st)) {
		owner_->GetWorldTransform().rotation =
			Quaternion::Multiply(baseRot_, Quaternion::Multiply(spinQ_, poseQ));
	}
}


bool PlayerDodgeMotion::IsDodging(DodgeState s) {
	return s == DodgeState::Startup || s == DodgeState::IFrame || s == DodgeState::Recovery;
}