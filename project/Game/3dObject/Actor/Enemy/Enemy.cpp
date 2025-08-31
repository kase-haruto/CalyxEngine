#include "Enemy.h"

#include <Engine/Foundation/Utility/Random/Random.h>
#include <Engine/Foundation/Clock/ClockManager.h>
#include <Engine/Objects/Collider/BoxCollider.h>
#include <Engine/Scene/Utility/SceneUtility.h>

#include <numbers>

/* ========================================================================
/* include space
/* ===================================================================== */

/////////////////////////////////////////////////////////////////////////////////////////
//		コンストラクタ
/////////////////////////////////////////////////////////////////////////////////////////
Enemy::Enemy(const std::string& modelName, const std::string objName)
	: Actor(modelName, objName) {
	worldTransform_.Initialize();
	worldTransform_.scale = { 2, 2, 2 };

	moveSpeed_ = Random::Generate<float>(1.0f, 3.0f);
	velocity_ = Random::GenerateVector3(-1.0f, 1.0f);

	collider_->SetType(ColliderType::Type_Enemy);
	collider_->SetTargetType(ColliderType::Type_PlayerAttack);
	collider_->SetOwner(this);
	if (auto* box = dynamic_cast<BoxCollider*>(collider_.get())) { box->SetSize({ 3, 3, 3 }); }
	collider_->SetIsDrawCollider(false);

	life_ = 1;
	waveAmplitude_ = 2.0f;
	waveSpeed_ = Random::Generate<float>(1.0f, 3.0f);

	hitFx_ = SceneAPI::Instantiate<ParticleSystemObject>("hitFx");
	hitFx_->LoadConfig("Resources/Assets/Configs/Effect/HitFx.json");

	explosionFx_ = SceneAPI::Instantiate<ParticleSystemObject>("explosionFx");
	explosionFx_->LoadConfig("Resources/Assets/Configs/Effect/Explosion.json");
}

/////////////////////////////////////////////////////////////////////////////////////////
//		デストラクタ
/////////////////////////////////////////////////////////////////////////////////////////
Enemy::~Enemy() {}

/////////////////////////////////////////////////////////////////////////////////////////
//		初期化処理
/////////////////////////////////////////////////////////////////////////////////////////
void Enemy::Initialize() {
	auto self = shared_from_this();

	hitFx_->SetParent(self);
	hitFx_->Stop();

	explosionFx_->SetParent(self);
	explosionFx_->Stop();
}

/////////////////////////////////////////////////////////////////////////////////////////
//		変換
/////////////////////////////////////////////////////////////////////////////////////////
static float Deg2Rad(float d) { return d * std::numbers::pi_v<float> / 180.0f; }

////////////////////////////////////////////////////////////////
//  Update
////////////////////////////////////////////////////////////////
void Enemy::Update(float dt) {
	/* =============================================
	   1. 生存中のロジック
	   =============================================*/
	if (deathState_ == DeathState::Alive) {
		if (life_ <= 0) {
			// ---- 死亡フラグ立った瞬間 ----
			//親子付け解除
			deathState_ = DeathState::Dying;
			explosionFx_->Play();
			deathTimer_ = 0.0f;
			deathRotateAxis_ = { 1, 0, 0 }; // 前方に倒れる
			return; // このフレームはここで終了
		}

		// 弾発射管理クラスの更新
		if (shootingController_) {
			shootingController_->SetGameplayEngaged(this->IsGameplayEngaged());

			if (this->IsGameplayEngaged()) {
				Shoot();
			}

			shootingController_->Update(dt);
		}

		// 波移動
		waveTime_ += dt * waveSpeed_;
		float offsetY = std::sin(waveTime_) * waveAmplitude_;
		worldTransform_.translation = basePosition_ + Vector3{ 0, offsetY, 0 };
		return;
	}

	/* =============================================
	   2. 倒れ演出中 (Dying)
	   =============================================*/
	if (deathState_ == DeathState::Dying) {
		deathTimer_ += dt;
		float t = std::clamp(deathTimer_ / deathLength_, 0.0f, 1.0f);

		// 0→90° まで補間して倒れる
		float rad = Deg2Rad(90.0f * t);
		worldTransform_.rotation =
			Quaternion::MakeRotateAxisQuaternion(deathRotateAxis_, rad);

		worldTransform_.translation = basePosition_; // 移動しない

		// 演出が終わり、爆発も再生終了したら Dead へ
		if (t >= 1.0f && !explosionFx_->IsPlaying()) {
			deathState_ = DeathState::Dead;
			deathTimer_ = 0.0f;
		}
		return;
	}

	/* =============================================
	   3. 完全に死亡 (Dead)
	   =============================================*/
	if (deathState_ == DeathState::Dead) {
		isAlive_ = false;
		return;
	}
}

////////////////////////////////////////////////////////////////
//  衝突
////////////////////////////////////////////////////////////////
void Enemy::OnCollisionEnter(Collider* other) {
	if (!other) return;
	if (collider_->GetTargetType() != other->GetType()) return;

	if (life_ >= 1) {
		life_--;
		hitFx_->Play();
	}
}

/////////////////////////////////////////////////////////////////////////////////////////
//		中心座標取得
/////////////////////////////////////////////////////////////////////////////////////////
const Vector3 Enemy::GetCenterPos() const {
	const Vector3 offset = { 0.0f, 1.5f, 0.0f };
	Vector3 worldPos = Vector3::Transform(offset, worldTransform_.matrix.world);
	return worldPos;
}

/////////////////////////////////////////////////////////////////////////////////////////
//		親の設定
/////////////////////////////////////////////////////////////////////////////////////////
void Enemy::SetParent(WorldTransform* parent) {
	worldTransform_.parent = parent;
	basePosition_ = worldTransform_.translation;
}

/////////////////////////////////////////////////////////////////////////////////////////
//		弾コントロール
/////////////////////////////////////////////////////////////////////////////////////////
void Enemy::SetShootingController(std::unique_ptr<EnemyShootingController> controller) {
	shootingController_ = std::move(controller);
}

void Enemy::SetPlayerTransform(const WorldTransform* tf) { playerTransform_ = tf; }

/////////////////////////////////////////////////////////////////////////////////////////
//		移動
/////////////////////////////////////////////////////////////////////////////////////////
void Enemy::Move() {}

/////////////////////////////////////////////////////////////////////////////////////////
//		弾発射
/////////////////////////////////////////////////////////////////////////////////////////
void Enemy::Shoot() {
	//playerの方向に弾を撃つ
	Vector3 myPos = GetCenterPos();
	Vector3 dir = Vector3(playerTransform_->GetWorldPosition() - myPos).Normalize();
	shootingController_->RequestShoot(GetCenterPos(), dir);
}
