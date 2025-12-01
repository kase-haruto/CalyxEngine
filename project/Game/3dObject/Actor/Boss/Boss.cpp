#include "Boss.h"

// engine
#include "Engine/Objects/3D/Actor/BaseGameObject.h"
#include <Engine/Foundation/Utility/Func/CxUtils.h>
#include <Engine/Objects/Collider/SphereCollider.h>

// game
#include "AI/BossAI.h"
#include "Anim/BossAnimController.h"
#include "Engine/Application/System/Enviroment.h"

/////////////////////////////////////////////////////////////////////////////////////////
//		ctor
/////////////////////////////////////////////////////////////////////////////////////////
Boss::Boss(const std::string& modelName, const std::string objName)
	: Actor(modelName, objName) {
	worldTransform_.Initialize();
	worldTransform_.scale = {30, 30, 30};

	moveSpeed_ = Random::Generate<float>(1.0f, 3.0f);
	velocity_  = Random::GenerateVector3(-1.0f, 1.0f);

	BaseGameObject::InitializeCollider(ColliderKind::Sphere);
	collider_->SetType(ColliderType::Type_Enemy);
	collider_->SetTargetType(ColliderType::Type_PlayerAttack);
	collider_->SetOwner(this);
	if(auto* sphere = dynamic_cast<SphereCollider*>(collider_.get())) {
		sphere->SetRadius(15.0f);
	}
	collider_->SetIsDrawCollider(true);

	life_ = 10;

	// アニメーションコントローラの生成
	AnimationModel* animModel = GetAnimationModel();
	anim_					  = std::make_unique<BossAnimController>(animModel);

	// ステートの初期化
	stateMachine_ = std::make_unique<BossStateMachine>();
	stateMachine_->SetOwner(this);
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
	Vector2 gaugePos = {kGameSize.x * 0.5f, 50.0f};
	hpGauge_->Initialize(gaugePos, Vector2(500.0f, 32.0f));
	hpGauge_->SetAncorPoint(Vector2(0.5f, 0.5f));
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
	if(deathState_ == DeathState::Alive) {
		if(life_ <= 0) {
			// ---- 死亡フラグ立った瞬間 ----
			// 親子付け解除
			deathState_		 = DeathState::Dying;
			deathRotateAxis_ = {1, 0, 0}; // 前方に倒れる
			return;						  // このフレームはここで終了
		}

		// 弾発射管理クラスの更新
		if(ai_) {
			ai_->Update(dt);
		}
		if(shootingController_) {
			shootingController_->Update(dt);
		}

		stateMachine_->Update(dt);

		// 方向合わせ（プレイヤーへ）
		{
			const Vector3 myPos		= GetWorldPosition();
			const Vector3 targetPos = target_ ? target_->GetWorldTransform().GetWorldPosition() : myPos;

			Vector3 d = targetPos - myPos;
			if(d.LengthSquared() > 1e-12f) {
				d = d.Normalize();

				const float yaw	  = std::atan2(d.x, d.z);								// 水平旋回
				const float pitch = std::atan2(-d.y, std::sqrt(d.x * d.x + d.z * d.z)); // 上下（LH）

				const Quaternion qWorld	 = Quaternion::MakeRotateY(yaw) * Quaternion::MakeRotateX(pitch);
				worldTransform_.rotation = qWorld;
			}
		}
	}

	if(deathState_ == DeathState::Dying) {
		return;
	}

	if(deathState_ == DeathState::Dead) {
		isAlive_ = false;
		return;
	}
}

/////////////////////////////////////////////////////////////////////////////////////////
//		デバッグgui描画
/////////////////////////////////////////////////////////////////////////////////////////
void Boss::DerivativeGui() {
	if(ImGui::CollapsingHeader("State")) {
		stateMachine_->ShowGui();
	}
	if(ImGui::CollapsingHeader("hpGauge")) {
		hpGauge_->ShowGui();
	}
}

/////////////////////////////////////////////////////////////////////////////////////////
//		衝突時処理
/////////////////////////////////////////////////////////////////////////////////////////
void Boss::OnCollisionEnter(Collider* other) {
	if(!other) return;
	if(collider_->GetTargetType() != other->GetType()) return;

	if(life_ >= 1) {
		life_--;

		// アイドル状態の時に攻撃を食らったら通知を送って状態遷移させる
		auto* curState = stateMachine_->GetCurrentState();
		if(curState && curState->GetStateType() == BossStateType::Idle) {
			isHit_ = true;
		}
	}
}

/////////////////////////////////////////////////////////////////////////////////////////
//		衝突終了処理
/////////////////////////////////////////////////////////////////////////////////////////
void Boss::OnCollisionExit(Collider* other) {
	(void)other;
	if(isHit_) isHit_ = false; // ダメージフラグ解除
}

/////////////////////////////////////////////////////////////////////////////////////////
//		中心座標取得
/////////////////////////////////////////////////////////////////////////////////////////
const Vector3 Boss::GetCenterPos() const {
	const Vector3 offset   = {0.0f, 1.5f, 0.0f};
	Vector3		  worldPos = Vector3::Transform(offset, worldTransform_.matrix.world);
	return worldPos;
}

#pragma region accessor
Vector3 Boss::GetTargetWorldPos() const { return target_? target_->GetWorldTransform().GetWorldPosition() : GetCenterPos(); }

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
const Actor* Boss::GetTargetActor() const {
	return target_;
}

/////////////////////////////////////////////////////////////////////////////////////////
//		アニメーター
/////////////////////////////////////////////////////////////////////////////////////////
BossAnimController* Boss::GetAnimator() const {
	return anim_.get();
}
/////////////////////////////////////////////////////////////////////////////////////////
//		AI取得
/////////////////////////////////////////////////////////////////////////////////////////
BossAI* Boss::GetAI() const {
	return ai_.get();
}

#pragma endregion