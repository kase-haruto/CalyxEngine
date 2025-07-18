#include "Enemy.h"

#include <Engine/Foundation/Utility/Random/Random.h>
#include <Engine/Foundation/Clock/ClockManager.h>
#include <Engine/Objects/Collider/BoxCollider.h>
#include <Engine/Scene/Utility/SceneUtility.h>

#include <numbers>

/* ========================================================================
/* include space
/* ===================================================================== */
Enemy::Enemy(const std::string& modelName, const std::string objName) :
	Actor::Actor(modelName, objName){

	worldTransform_.Initialize();
	worldTransform_.scale = {2.0f, 2.0f, 2.0f};
	moveSpeed_ = Random::Generate<float>(1.0f, 3.0f);
	velocity_ = Random::GenerateVector3(-1.0f, 1.0f);

	collider_->SetType(ColliderType::Type_Enemy);
	collider_->SetTargetType(ColliderType::Type_PlayerAttack);
	collider_->SetOwner(this);
	auto* boxCollider = dynamic_cast< BoxCollider* >(collider_.get());
	boxCollider->SetSize(Vector3(3.0f, 3.0f, 3.0f));

	collider_->SetIsDrawCollider(false);

	life_ = 2;

	waveAmplitude_ = 2.0f;
	waveSpeed_ = Random::Generate<float>(1.0f, 3.0f);

	hitFx_ = SceneAPI::Instantiate<ParticleSystemObject>("hitFx");
	hitFx_->LoadConfig("Resources/Assets/Configs/Effect/HitFx.json");
	
	explosionFx_ = SceneAPI::Instantiate<ParticleSystemObject>("explosionFx");
	explosionFx_->LoadConfig("Resources/Assets/Configs/Effect/Explosion.json");
	
}

/////////////////////////////////////////////////////////////////////////////////////////
//		初期化
/////////////////////////////////////////////////////////////////////////////////////////
void Enemy::Initialize(){
	auto self = shared_from_this();

	hitFx_->SetParent(self);
	hitFx_->Stop();

	explosionFx_->SetParent(self);
	explosionFx_->Stop();
}


float DegreesToRadians(float degrees){
	return degrees * (std::numbers::pi_v<float> / 180.0f);
}

/////////////////////////////////////////////////////////////////////////////////////////
//		更新
/////////////////////////////////////////////////////////////////////////////////////////
void Enemy::Update(){
	float dt = ClockManager::GetInstance()->GetDeltaTime();

	if (life_ <= 0){
		if (!isDead_){
			explosionFx_->Play();
			isAlive_ = false;
			isDead_ = true;
			return;
		}

		if (isDead_){
			deathRotation_ += dt * 90.0f; // 秒間90度
			float maxAngle = 90.0f;
			if (deathRotation_ > maxAngle){
				deathRotation_ = maxAngle;
			}

			// 【自作クォータニオン + 自作度→ラジアン変換】
			float radian = DegreesToRadians(deathRotation_);
			worldTransform_.rotation = Quaternion::MakeRotateAxisQuaternion(deathRotateAxis_, radian);
		}

		worldTransform_.translation = basePosition_; // 死亡時は波移動なし
		worldTransform_.Update();

		if (!explosionFx_->isPlayng()){
			isAlive_ = false;
		}

		return;
	}

	// 生存時
	waveTime_ += dt * waveSpeed_;
	float offsetY = std::sin(waveTime_) * waveAmplitude_;
	worldTransform_.translation = basePosition_ + Vector3(0.0f, offsetY, 0.0f);

	worldTransform_.Update();

	BaseGameObject::Update();
}
void Enemy::OnCollisionEnter([[maybe_unused]] Collider* other){
	if (!other) return;

	// 自分のターゲットと相手のタイプが一致するか
	if (collider_->GetTargetType() == other->GetType()){
		// 相手がターゲットだった場合の処理
		life_--;
		//hitFx_->Play();
	}
}

const Vector3 Enemy::GetCenterPos() const{
	const Vector3 offset = {0.0f, 1.5f, 0.0f};
	Vector3 worldPos = Vector3::Transform(offset, worldTransform_.matrix.world);
	return worldPos;
}

void Enemy::SetParent(WorldTransform* parent){
	worldTransform_.parent = parent;
	basePosition_ = worldTransform_.translation;
}

/////////////////////////////////////////////////////////////////////////////////////////
//		移動
/////////////////////////////////////////////////////////////////////////////////////////
void Enemy::Move(){}

/////////////////////////////////////////////////////////////////////////////////////////
//		弾発射
/////////////////////////////////////////////////////////////////////////////////////////
void Enemy::Shoot(){}
