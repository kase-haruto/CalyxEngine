#include "Boss.h"

// engine
#include "Engine/Objects/3D/Actor/BaseGameObject.h"
#include <Engine/Foundation/Utility/Func/CxUtils.h>
#include <Engine/Objects/Collider/SphereCollider.h>

// game
#include "AI/BossAI.h"
#include "Attack/BossNormalShoot.h"

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

	life_          = 10;
	waveAmplitude_ = 2.0f;
	waveSpeed_     = Random::Generate<float>(1.0f,3.0f);
}

/////////////////////////////////////////////////////////////////////////////////////////
//		デストラクタ
/////////////////////////////////////////////////////////////////////////////////////////
Boss::~Boss() {}

/////////////////////////////////////////////////////////////////////////////////////////
//		初期化
/////////////////////////////////////////////////////////////////////////////////////////
void Boss::Initialize() {
	// コンフィグの読み込みと適用
	config_.LoadConfig(configRoot_ + "Boss");

}

void Boss::InitializeAI() {
	ai_ = std::make_unique<BossAI>(this,shootingController_.get());

	//攻撃の追加
	ai_->AddAttack(std::make_unique<BossNormalShoot>());
}

/////////////////////////////////////////////////////////////////////////////////////////
//		更新処理
/////////////////////////////////////////////////////////////////////////////////////////
void Boss::Update(float dt) {
	/* =============================================
		1. 生存中のロジック
	=============================================*/
	if(deathState_ == DeathState::Alive) {
		if(life_ <= 0) {
			// ---- 死亡フラグ立った瞬間 ----
			//親子付け解除
			deathState_      = DeathState::Dying;
			deathTimer_      = 0.0f;
			deathRotateAxis_ = {1,0,0}; // 前方に倒れる
			return;                     // このフレームはここで終了
		}

		// 弾発射管理クラスの更新
		if(ai_) { ai_->Update(dt); }
		if(shootingController_) { shootingController_->Update(dt); }

		// 方向合わせ（プレイヤーへ）
		{
			const Vector3 myPos     = GetWorldPosition();
			const Vector3 targetPos = playerTransform_ ? playerTransform_->GetWorldPosition() : myPos;

			Vector3 d = targetPos - myPos;
			if(d.LengthSquared() > 1e-12f) {
				d = d.Normalize();

				const float yaw   = std::atan2(d.x,d.z);                               // 水平旋回
				const float pitch = std::atan2(-d.y,std::sqrt(d.x * d.x + d.z * d.z)); // 上下（LH）

				const Quaternion qWorld  = Quaternion::MakeRotateY(yaw) * Quaternion::MakeRotateX(pitch);
				worldTransform_.rotation = qWorld;
			}
		}

		// 波移動
		waveTime_ += dt * waveSpeed_;
		float offsetY               = std::sin(waveTime_) * waveAmplitude_;
		worldTransform_.translation = basePosition_ + Vector3{0,offsetY,0};
		return;
	}

	/* =============================================
	   2. 倒れ演出中 (Dying)
	   =============================================*/
	if(deathState_ == DeathState::Dying) {
		deathTimer_ += dt;
		float t = std::clamp(deathTimer_ / deathLength_,0.0f,1.0f);

		// 0→90° まで補間して倒れる
		float rad                = Cx::Math::ToRadians(90.0f * t);
		worldTransform_.rotation =
			Quaternion::MakeRotateAxisQuaternion(deathRotateAxis_,rad);

		worldTransform_.translation = basePosition_; // 移動しない

		// 演出が終わり、爆発も再生終了したら Dead へ
		if(t >= 1.0f) {
			deathState_ = DeathState::Dead;
			deathTimer_ = 0.0f;
		}
		return;
	}

	/* =============================================
	   3. 完全に死亡 (Dead)
	   =============================================*/
	if(deathState_ == DeathState::Dead) {
		isAlive_ = false;
		return;
	}
}

/////////////////////////////////////////////////////////////////////////////////////////
//		衝突時処理
/////////////////////////////////////////////////////////////////////////////////////////
void Boss::OnCollisionEnter(Collider* other) {
	if(!other) return;
	if(collider_->GetTargetType() != other->GetType()) return;

	if(life_ >= 1) { life_--; }
}

/////////////////////////////////////////////////////////////////////////////////////////
//		中心座標取得
/////////////////////////////////////////////////////////////////////////////////////////
const Vector3 Boss::GetCenterPos() const {
	const Vector3 offset   = {0.0f,1.5f,0.0f};
	Vector3       worldPos = Vector3::Transform(offset,worldTransform_.matrix.world);
	return worldPos;
}

Vector3 Boss::GetTargetWorldPos() const { return playerTransform_ ? playerTransform_->GetWorldPosition() : GetCenterPos(); }

/////////////////////////////////////////////////////////////////////////////////////////
//		発射制御クラスの取得
/////////////////////////////////////////////////////////////////////////////////////////
void Boss::SetShootingController(std::unique_ptr<BossShootingController> controller) { shootingController_ = std::move(controller); }

void Boss::SetPlayerTransform(const WorldTransform* tf) { playerTransform_ = tf; }