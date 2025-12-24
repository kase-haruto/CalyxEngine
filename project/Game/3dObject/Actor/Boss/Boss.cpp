#include "Boss.h"

// engine
#include "Engine/Objects/3D/Actor/BaseGameObject.h"
#include <Engine/Foundation/Utility/Func/CxUtils.h>
#include <Engine/Objects/Collider/SphereCollider.h>

// game
#include "AI/BossAI.h"
#include "Anim/BossAnimController.h"
#include "Engine/Application/System/Enviroment.h"
#include "Engine/Scene/Utility/SceneUtility.h"

/////////////////////////////////////////////////////////////////////////////////////////
//		ctor
/////////////////////////////////////////////////////////////////////////////////////////
Boss::Boss(const std::string& modelName,const std::string objName)
	: Actor(modelName,objName) {
	worldTransform_.Initialize();
	worldTransform_.scale = {30,30,30};

	moveSpeed_ = Random::Generate<float>(1.0f,3.0f);
	velocity_  = Random::GenerateVector3(-1.0f,1.0f);

	BaseGameObject::InitializeCollider(ColliderKind::Sphere);
	collider_->SetType(ColliderType::Type_Enemy);
	collider_->SetTargetType(ColliderType::Type_PlayerAttack);
	collider_->SetOwner(this);
	if(auto* sphere = dynamic_cast<SphereCollider*>(collider_.get())) { sphere->SetRadius(15.0f); }
	collider_->SetIsDrawCollider(true);

	life_ = 35;

	// アニメーションコントローラの生成
	CalyxAssets::AnimationModel* animModel = AnimationModel();
	anim_                     = std::make_unique<BossAnimController>(animModel);

	// ステートの初期化
	stateMachine_ = std::make_unique<BossStateMachine>();
	stateMachine_->SetOwner(this);

	// --- FxObject を生成して再生 ---
	hitEffects_ = SceneAPI::Instantiate<CalyxEffect::FxObject>("HitFx");

	// コンフィグ読み込み（FxObject 内部で ApplyConfig が呼ばれる）
	auto fx = hitEffects_.lock();
	fx->LoadFromPath("Effect/BossHitEffect");
}

/////////////////////////////////////////////////////////////////////////////////////////
//		デストラクタ
/////////////////////////////////////////////////////////////////////////////////////////
Boss::~Boss() = default;

/////////////////////////////////////////////////////////////////////////////////////////
//		初期化
/////////////////////////////////////////////////////////////////////////////////////////
void Boss::Initialize() {
	// コンフィグの読み込みと適用
	config_.LoadConfig(configRoot_ + "Boss");

	// アニメーション初期化
	anim_->Initialize();
	hpGauge_ = std::make_unique<HpGauge>(static_cast<float>(life_));
	// 画面中央上にゲージを設定
	CalyxMath::Vector2 gaugePos = {kGameSize.x * 0.5f,50.0f};
	hpGauge_->Initialize(gaugePos,CalyxMath::Vector2(500.0f,32.0f));
	hpGauge_->SetAncorPoint(CalyxMath::Vector2(0.5f,0.5f));

	auto fx = hitEffects_.lock();
	fx->StopAll();
}

void Boss::InitializeAI() {
	ai_ = std::make_unique<BossAI>(this);

	stateMachine_->SetInitialState(BossStateType::Idle);
}

/////////////////////////////////////////////////////////////////////////////////////////
//		更新処理
/////////////////////////////////////////////////////////////////////////////////////////
void Boss::Update(float dt) {
	if(hpGauge_) {
		hpGauge_->Update(dt);
		hpGauge_->SetHp(static_cast<float>(life_));
	}

	// 弾発射管理クラスの更新
	if(ai_) { ai_->Update(dt); }
	if(shootingController_) { shootingController_->Update(dt); }

	stateMachine_->Update(dt);

	// 方向合わせ（プレイヤーへ）
	{
		const CalyxMath::Vector3 myPos     = GetWorldPosition();
		const CalyxMath::Vector3 targetPos = target_ ? target_->GetWorldTransform().GetWorldPosition() : myPos;

		CalyxMath::Vector3 d = targetPos - myPos;
		if(d.LengthSquared() > 1e-12f) {
			d = d.Normalize();

			const float yaw   = std::atan2(d.x,d.z);                               // 水平旋回
			const float pitch = std::atan2(-d.y,std::sqrt(d.x * d.x + d.z * d.z)); // 上下（LH）

			const CalyxMath::Quaternion qWorld  = CalyxMath::Quaternion::MakeRotateY(yaw) * CalyxMath::Quaternion::MakeRotateX(pitch);
			worldTransform_.rotation = qWorld;
		}
	}

	// 死亡処理
	if(stateMachine_->GetCurrentStateType() == BossStateType::Dead) {
		// 死亡アニメーションが終わったら死亡フラグを立てる
		if(anim_->IsAnimFinished()) { isAlive_ = false; }
	}
}

/////////////////////////////////////////////////////////////////////////////////////////
//		デバッグgui描画
/////////////////////////////////////////////////////////////////////////////////////////
void Boss::DerivativeGui() {
	if(ImGui::CollapsingHeader("State")) { stateMachine_->ShowGui(); }
	if(ImGui::CollapsingHeader("hpGauge")) { hpGauge_->ShowGui(); }
}

/////////////////////////////////////////////////////////////////////////////////////////
//		衝突時処理
/////////////////////////////////////////////////////////////////////////////////////////
void Boss::OnCollisionEnter(Collider* other) {
	if (!other) return;

	// 攻撃以外を除外
	if (collider_->GetTargetType() != other->GetType()) return;
	if (life_ <= 0) return;

	// --- ダメージ処理 ---
	life_--;

	// --- 衝突位置を取得 ---
	CalyxMath::Vector3 hitPos = other->GetWorldPos(); // ← マジでこれだけでOKのことが多い

	auto fx = hitEffects_.lock();
	// 位置設定
	fx->SetWorldPosition(hitPos);
	// 再生
	fx->PlayAll();

	// --- HP0 なら Dead へ ---
	if (life_ <= 0) {
		stateMachine_->ChangeState(BossStateType::Dead);
		return;
	}

	// --- ひるみ処理 ---
	if (flinchValue_ < flinchMax_) {
		flinchValue_++;
		return;
	}
	flinchValue_ = 0;

	auto* curState = stateMachine_->GetCurrentState();
	if (curState && curState->GetStateType() == BossStateType::Idle) {
		stateMachine_->ChangeState(BossStateType::Damage);
	}
}

/////////////////////////////////////////////////////////////////////////////////////////
//		衝突終了処理
/////////////////////////////////////////////////////////////////////////////////////////
void Boss::OnCollisionExit([[maybe_unused]]Collider* other) {  }

/////////////////////////////////////////////////////////////////////////////////////////
//		中心座標取得
/////////////////////////////////////////////////////////////////////////////////////////
const CalyxMath::Vector3 Boss::GetCenterPos() const {
	const CalyxMath::Vector3 offset   = {0.0f,1.5f,0.0f};
	CalyxMath::Vector3       worldPos = CalyxMath::Vector3::Transform(offset,worldTransform_.matrix.world);
	return worldPos;
}

#pragma region accessor
CalyxMath::Vector3 Boss::GetTargetWorldPos() const { return target_ ? target_->GetWorldTransform().GetWorldPosition() : GetCenterPos(); }

/////////////////////////////////////////////////////////////////////////////////////////
//		発射制御クラスの取得
/////////////////////////////////////////////////////////////////////////////////////////
void Boss::SetShootingController(std::unique_ptr<BossShootingController> controller) { shootingController_ = std::move(controller); }

/////////////////////////////////////////////////////////////////////////////////////////
//		プレイヤーのTransformを設定
/////////////////////////////////////////////////////////////////////////////////////////
void Boss::SetPlayerTransform(const Actor* target) { target_ = target; }

std::vector<Sprite*> Boss::GetAllSprites() const {
	std::vector<Sprite*> sprites;
	// ライフゲージ
	if(hpGauge_) {
		sprites.push_back(hpGauge_->GetFrameSprite());
		sprites.push_back(hpGauge_->GetDamageGauge());
		sprites.push_back(hpGauge_->GetMainGauge());
	}
	return sprites;
}

const Actor* Boss::GetTargetActor() const { return target_; }

/////////////////////////////////////////////////////////////////////////////////////////
//		アニメーター
/////////////////////////////////////////////////////////////////////////////////////////
BossAnimController* Boss::GetAnimator() const { return anim_.get(); }
/////////////////////////////////////////////////////////////////////////////////////////
//		AI取得
/////////////////////////////////////////////////////////////////////////////////////////
BossAI* Boss::GetAI() const { return ai_.get(); }

#pragma endregion