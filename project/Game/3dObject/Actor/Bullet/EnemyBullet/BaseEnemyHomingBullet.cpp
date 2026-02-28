#include "BaseEnemyHomingBullet.h"
/* ========================================================================
/* include space
/* ===================================================================== */
#include "Engine/Scene/Context/SceneContext.h"
#include "Game/3d/GameCamera/RailCamera.h"

#include <Engine/Foundation/Utility/Random/Random.h>
#include <Engine/Graphics/Camera/Manager/CameraManager.h>
#include <Engine/Objects/Collider/BoxCollider.h>

#include <algorithm>
#include <cmath>
#include <numbers>

namespace {
	inline float			  Dot(const CalyxMath::Vector3& a, const CalyxMath::Vector3& b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
	inline CalyxMath::Vector3 Cross(const CalyxMath::Vector3& a, const CalyxMath::Vector3& b) {
		return {a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x};
	}
	inline CalyxMath::Vector3 NormalizeSafe(const CalyxMath::Vector3& v, const CalyxMath::Vector3& fallback = {0, 0, 1}) {
		float len = v.Length();
		if(len <= 1e-6f) return fallback;
		return v / len;
	}
	inline float SafeAcos(float x) {
		if(x < -1.0f) x = -1.0f;
		if(x > 1.0f) x = 1.0f;
		return std::acos(x);
	}
	inline void MakeOrthoBasis(const CalyxMath::Vector3& los, CalyxMath::Vector3& side, CalyxMath::Vector3& up) {
		CalyxMath::Vector3 n = NormalizeSafe(los);
		CalyxMath::Vector3 a = (std::fabs(n.y) < 0.99f) ? CalyxMath::Vector3{0, 1, 0} : CalyxMath::Vector3{1, 0, 0};
		side				 = NormalizeSafe(Cross(n, a));
		up					 = NormalizeSafe(Cross(side, n));
	}
	inline CalyxMath::Vector3 RotateAroundAxis(const CalyxMath::Vector3& v, const CalyxMath::Vector3& axisUnit, float angle) {
		float			   c = std::cos(angle), s = std::sin(angle);
		CalyxMath::Vector3 k	 = axisUnit;
		CalyxMath::Vector3 kxv	 = Cross(k, v);
		float			   kdotv = Dot(k, v);
		return v * c + kxv * s + k * (kdotv * (1.0f - c));
	}
} // namespace

///////////////////////////////////////////////////////////////////////////////////////////
//		ctor / dtor
///////////////////////////////////////////////////////////////////////////////////////////
BaseEnemyHomingBullet::BaseEnemyHomingBullet(const std::string& modelName, const std::string& name)
	: BaseBullet::BaseBullet(modelName, name) {
	SetDrawEnable(true);

	param_->LoadParams();
	moveSpeed_ = 25.0f;
}

///////////////////////////////////////////////////////////////////////////////////////////
//		初期化
///////////////////////////////////////////////////////////////////////////////////////////
void BaseEnemyHomingBullet::ShootInitialize(const CalyxMath::Vector3& initPos, const CalyxMath::Vector3& velocity) {
	CalyxMath::Vector3 initDir = (velocity.Length() > 0.001f) ? velocity.Normalize() : CalyxMath::Vector3(0, 0, 1);
	BaseBullet::ShootInitialize(initPos, initDir * param_->homingSpeed);
	time_			  = 0.0f;
	homingElapsedSec_ = 0.0f;
}

void BaseEnemyHomingBullet::Initialize() {
	collider_->SetType(ColliderType::Type_EnemyAttack);
	collider_->SetTargetType(ColliderType::Type_Player);
	collider_->SetOwner(this);
	if(auto* box = dynamic_cast<BoxCollider*>(collider_.get())) {
		box->SetSize({1.5f, 1.5f, 1.5f});
	}
	BaseGameObject::SetTexture("white1x1.png");
	BaseGameObject::SetBillboardMode(BillboardMode::Full);

	// ライティングなし
	BaseGameObject::SetLightingMode(LightingMode::UnlitColor);
	BaseGameObject::SetBlendMode(BlendMode::ADD);
	CalyxMath::Vector3 rgb = Random::GenerateVector3(0.0f, 1.0f);
	CalyxMath::Vector3 color{rgb};
	model_->SetColor(color);

	baseScale_ = worldTransform_.scale;

	auto self = shared_from_this();
	if(auto fx = trailFx_.lock()) {
		fx->SetParent(self);
		fx->StopAll();
	}
}

void BaseEnemyHomingBullet::OnShot() {
	if(auto fx = trailFx_.lock()) {
		fx->PlayAll();
	}
}

///////////////////////////////////////////////////////////////////////////////////////////
//		更新
///////////////////////////////////////////////////////////////////////////////////////////
void BaseEnemyHomingBullet::Update(float dt) {
	time_ += dt;
	homingElapsedSec_ += dt;

	// --- 最初の homingDurationSec 秒だけ誘導 ---
	if(homingElapsedSec_ < param_->homingDurationSec) {
		if(TARGET_IS_NT102_OR_LATER) {
			const CalyxMath::Vector3 selfPos = GetCenterPos();
			CalyxMath::Vector3		 tgtPos;

			// targetPositionを使う
			if(targetPosition_.has_value()) {
				tgtPos = targetPosition_.value();
			} else if(target_) {
				tgtPos = target_->GetWorldPosition();
			} else {
				// fallback
				tgtPos = selfPos + CalyxMath::Vector3(0, 0, 1);
			}

			CalyxMath::Vector3 los = tgtPos - selfPos;
			CalyxMath::Vector3 side, up;
			MakeOrthoBasis(los, side, up);

			velocity_ = CalculateHomingVelocity(
				velocity_,
				tgtPos,
				dt,
				param_->homingSpeed,
				param_->rotateSpeed,
				param_->guidance);
		}
	}

	// --- scale をうねうね揺らす ---
	float sx = 1.0f + param_->scaleAmp * std::sin(time_ * param_->scaleFreq);
	float sy = 1.0f + param_->scaleAmp * std::cos(time_ * param_->scaleFreq);

	// 基準スケールに対して相対的に揺らす
	worldTransform_.scale.x = baseScale_.x * sx;
	worldTransform_.scale.y = baseScale_.y * sy;
	worldTransform_.scale.z = baseScale_.z;

	BaseBullet::Update(dt);

	// ---- カメラ距離によるエフェクトのフェード ----
	if(auto fx = trailFx_.lock()) {
		if(auto cam = SceneContext::Current()->FindFirst<RailCamera>()) {
			// Shader側で10m〜50mのフェードを実行
			fx->SetCameraFade(10.0f, 50.0f);
			fx->SetDrawEnable(true);

			// 密度スケーリングのために AlphaMultiplier も反映
			const CalyxMath::Vector3 camPos = cam->GetWorldTransform().GetWorldPosition();
			const CalyxMath::Vector3 myPos	= GetCenterPos();
			float					 dist	= (myPos - camPos).Length();

			float alpha = std::clamp((dist - 10.0f) / (50.0f - 10.0f), 0.0f, 1.0f);
			fx->SetAlphaMultiplier(alpha);
		}
	}
}

///////////////////////////////////////////////////////////////////////////////////////////
//		helper
///////////////////////////////////////////////////////////////////////////////////////////
void BaseEnemyHomingBullet::SetTarget(const WorldTransform* target) {
	target_ = target;
}

const CalyxMath::Vector3 BaseEnemyHomingBullet::GetCenterPos() const {
	const CalyxMath::Vector3 offset = {0.0f, 1.0f, 0.0f};
	return CalyxMath::Vector3::Transform(offset, worldTransform_.matrix.world);
}

CalyxMath::Vector3 BaseEnemyHomingBullet::TurnTowards(
	const CalyxMath::Vector3& currentDir,
	const CalyxMath::Vector3& targetDir,
	float					  maxRad) const {
	CalyxMath::Vector3 from	  = NormalizeSafe(currentDir);
	CalyxMath::Vector3 to	  = NormalizeSafe(targetDir, from);
	float			   cosang = Dot(from, to);
	float			   ang	  = SafeAcos(cosang);
	if(ang <= 1e-6f) return to;
	if(maxRad >= ang) return to;
	CalyxMath::Vector3 axis = NormalizeSafe(Cross(from, to), {0, 1, 0});
	return NormalizeSafe(RotateAroundAxis(from, axis, maxRad));
}

CalyxMath::Vector3 BaseEnemyHomingBullet::CalculateHomingVelocity(
	const CalyxMath::Vector3& currentVel,
	const CalyxMath::Vector3& targetPos,
	float					  dt,
	float					  speed,
	float					  rotateSpeedDegPerSec,
	float					  guidance) const {
	const CalyxMath::Vector3 selfPos	= GetCenterPos();
	const CalyxMath::Vector3 desiredDir = NormalizeSafe(targetPos - selfPos);
	const CalyxMath::Vector3 currentDir = NormalizeSafe(currentVel, CalyxMath::Vector3{0, 0, 1});

	const float				 maxRad		= (rotateSpeedDegPerSec * static_cast<float>(std::numbers::pi) / 180.0f) * dt;
	const CalyxMath::Vector3 clampedDir = TurnTowards(currentDir, desiredDir, maxRad);
	const CalyxMath::Vector3 newDir		= NormalizeSafe(currentDir * (1.0f - guidance) + clampedDir * guidance, currentDir);

	return newDir * speed;
}

EnemyHomingBulletParam::EnemyHomingBulletParam() {

	AddField("homingSpeed", homingSpeed).Category("HomingBasic").Tooltip("ホーミング弾の速度");
	AddField("rotateSpeed", rotateSpeed).Category("HomingBasic").Tooltip("ホーミングの旋回速度(度/秒)");
	AddField("guidance", guidance).Category("HomingBasic").Range(0.0f, 1.0f).Tooltip("ホーミングの誘導率(0～1)");
	AddField("homingDurationSec", homingDurationSec).Category("HomingBasic").Tooltip("ホーミング誘導が続く時間(秒)");
}

CalyxEngine::ParamPath EnemyHomingBulletParam::GetParamPath() const {
	return {CalyxEngine::ParamDomain::Game, "EnemyHomingBullet", subRoot};
}
