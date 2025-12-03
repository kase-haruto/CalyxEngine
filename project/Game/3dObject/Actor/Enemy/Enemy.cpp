#include "Enemy.h"

// engine
#include <Engine/Foundation/Utility/Random/Random.h>
#include <Engine/Objects/Collider/SphereCollider.h>
#include <Engine/Scene/Utility/SceneUtility.h>

// game
#include <Game/Battle/Shooting/Details/AimProvider.h>
#include <Game/Battle/Shooting/Details/FireScheduler.h>
#include <Game/Battle/Shooting/Score/GainScore.h>
#include <Game/Battle/Shooting/ShootingController/EnemyShootingControllerSink.h>

#include <numbers>

Enemy::Enemy() = default;

Enemy::Enemy(const std::string& modelName, const std::string objName)
	: Actor(modelName, objName) {

	worldTransform_.scale = {2, 2, 2};

	BaseGameObject::InitializeCollider(ColliderKind::Sphere);
	if(auto* c = dynamic_cast<SphereCollider*>(collider_.get())) {
		c->SetRadius(1.5f);
	}
	collider_->SetType(ColliderType::Type_Enemy);
	collider_->SetTargetType(ColliderType::Type_PlayerAttack);
	collider_->SetOwner(this);

	// hit effect
	hitFx_ = SceneAPI::Instantiate<FxObject>("HitFx");
	hitFx_->LoadFromPath("Effect/HitEffect");
}

Enemy::~Enemy() {}

void Enemy::Initialize() {
	auto self = shared_from_this();
	hitFx_->SetParent(self);
	hitFx_->StopAll();

	// movement controller 初期化
	movement_.Initialize(this);

	EnsurePatternBound();
}

void Enemy::StartStayInCamera(float duration) {
	movement_.StartStay(duration);
}

void Enemy::SetPlayerTransform(const WorldTransform* tf) {
	playerTransform_ = tf;
	movement_.SetPlayerTransform(tf);
}

void Enemy::SetRouteSpline(const SplineData& data) {
	movement_.SetRoute(data, playerTransform_);
}

void Enemy::Update(float dt) {

	// death check
	if(deathState_ == DeathState::Alive) {
		if(life_ <= 0) {
			deathState_		 = DeathState::Dying;
			deathTimer_		 = 0.0f;
			deathRotateAxis_ = {1, 0, 0};
			return;
		}
	}

	// movement
	movement_.Update(dt);

	// shooting
	BuildEmitterIfReady();

	if(shootingController_) {
		shootingController_->SetGameplayEngaged(IsGameplayEngaged());
		shootingController_->Update(dt);
	}

	if(IsGameplayEngaged() && emitter_) {
		if(auto* pat = emitter_->Pattern()) pat->Advance(dt);

		BulletEmitterContext cxt{};
		cxt.origin	  = GetCenterPos();
		cxt.targetPos = playerTransform_
							? playerTransform_->GetWorldPosition()
							: GetWorldPosition();

		emitter_->Update(dt, cxt);
	}

	// death anim
	if(deathState_ == DeathState::Dying) {

		deathTimer_ += dt;
		float t = std::clamp(deathTimer_ / deathLength_, 0.0f, 1.0f);

		float rad = std::numbers::pi_v<float> * 0.5f * t;
		worldTransform_.rotation =
			Quaternion::MakeRotateAxisQuaternion(deathRotateAxis_, rad);

		if(t >= 1.0f) {
			deathState_ = DeathState::Dead;
			deathTimer_ = 0.0f;
		}
		return;
	}

	if(deathState_ == DeathState::Dead) {
		Die();
		isAlive_ = false;
		return;
	}
}

void Enemy::BuildEmitterIfReady() {
	if(emitter_) return;
	if(!shootingController_) return;
	if(!playerTransform_) return;

	auto sink = std::make_unique<EnemyShootingControllerSink>(shootingController_.get());
	auto aim  = std::make_unique<AimAtTarget>();

	FireScheduler sched;
	sched.shotsPerSec = 1.5f;

	BulletEmitterConfig cfg;
	cfg.tag = "enemy_homing";

	emitter_ = std::make_unique<BulletEmitter>(
		cfg, std::move(sink), std::move(aim), nullptr, sched);

	EnsurePatternBound();
}

void Enemy::EnsurePatternBound() {
	if(!emitter_) return;

	if(!pattern_ || patternKind_ != lastPatternKind_) {
		pattern_		 = CreatePattern(patternKind_);
		lastPatternKind_ = patternKind_;
	}

	emitter_->SetPattern(pattern_.get());
}

void Enemy::SetShootingController(std::unique_ptr<EnemyShootingController> controller) {
	shootingController_ = std::move(controller);
}

void Enemy::OnCollisionEnter(Collider*) {
	if(life_ >= 1) {
		life_--;
		hitFx_->PlayAll();
	}
}

const Vector3 Enemy::GetCenterPos() const {
	const Vector3 offset{0, 1.5f, 0};
	return Vector3::Transform(offset, worldTransform_.matrix.world);
}

int16_t Enemy::GetScore() const { return score_; }

void Enemy::Die() {
	GainScore e;
	e.amount = score_;
	e.reason = "enemyKill";
	EventBus::Publish(e);
}
