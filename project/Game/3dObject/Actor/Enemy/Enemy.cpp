#include "Enemy.h"

// engine
#include <Engine/Foundation/Utility/Random/Random.h>
#include <Engine/Objects/Collider/SphereCollider.h>
#include <Engine/Scene/Utility/SceneUtility.h>

// game
#include <Game/Battle/Shooting/Score/GainScore.h>

#include <numbers>

Enemy::Enemy() = default;

Enemy::Enemy(const std::string& modelName, const std::string& objName)
	: EnemyFactionActor(modelName, objName) {

	// ---- EnemyFactionActor 設定 ----
	SetEnemyKind(EnemyKind::Normal);
	
	BaseGameObject::InitializeCollider(ColliderKind::Sphere);
	if(auto* c = dynamic_cast<SphereCollider*>(collider_.get())) {
		c->SetRadius(param_.col.radius);
	}
	collider_->SetType(ColliderType::Type_Enemy);
	collider_->SetTargetType(ColliderType::Type_PlayerAttack);
	collider_->SetOwner(this);

	// hit effect
	hitFx_ = SceneAPI::Instantiate<CalyxEffect::FxObject>("HitFx");
	hitFx_->LoadFromPath("Effect/HitEffect");

	// パラメータのロード
	InitializeSerializableParm();
}

Enemy::~Enemy() = default;

void Enemy::Initialize() {
	auto self = shared_from_this();
	hitFx_->SetParent(self);
	hitFx_->StopAll();

	// movement / shooting 初期化
	movement_.Initialize(this);
	shooting_.Initialize(this);

}

void Enemy::SetPlayerTransform(const WorldTransform* tf) {
	playerTransform_ = tf;
	movement_.SetPlayerTransform(tf);
	shooting_.SetTarget(tf);
}

void Enemy::SetRouteSpline(const SplineData& data) {
	movement_.SetRoute(data, playerTransform_);
}

void Enemy::SetShootingController(std::unique_ptr<EnemyShootingController> controller) {
	shooting_.SetController(std::move(controller));
}

void Enemy::Update(float dt) {

	// death check
	if(deathState_ == DeathState::Alive) {
		if(life_ <= 0) {
			deathState_		 = DeathState::Dying;
			deathTimer_		 = 0.0f;
			deathRotateAxis_ = param_.death.rotateAxis;
			return;
		}
	}

	// movement
	movement_.Update(dt);

	// 解散行動中は射撃しない
	if(movement_.GetMode() != EnemyMovementController::Mode::Dissolving) {
		if(deathState_ == DeathState::Alive) {
		shooting_.Update(dt);
		}
	} else {
		// 解散行動中に色を薄くしていく
		float	subAlpha = 0.5f * dt;
		CalyxMath::Vector4 col		 = GetModel()->GetColor();
		col.w			 = (std::max)(0.0f, col.w - subAlpha);
		GetModel()->SetColor(col);


	}

	// 完全に透明になったら即死
	if(GetModel()->GetColor().w <= 0.01f) {
		isAlive_ = false;
		return;
	}

	// death anim
	if(deathState_ == DeathState::Dying) {

		deathTimer_ += dt;
		float t = std::clamp(deathTimer_ / param_.death.length, 0.0f, 1.0f);

		float rad = std::numbers::pi_v<float> * 0.5f * t;
		worldTransform_.rotation =
			CalyxMath::Quaternion::MakeRotateAxisQuaternion(deathRotateAxis_, rad);

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

void Enemy::StartEntranceToFormation(
	EnemyFormationController* formation,
	const CalyxMath::Vector3&			  offset,
	const CalyxMath::Vector3&			  entranceStart) {
	movement_.StartEntranceToFormation(
		formation,
		offset,
		entranceStart);
}

void Enemy::OnCollisionEnter(Collider*) {
	if(life_ >= 1) {
		life_--;
		hitFx_->PlayAll();
	}
}

const CalyxMath::Vector3 Enemy::GetCenterPos() const {
	const CalyxMath::Vector3 offset{0, 1.5f, 0};
	return CalyxMath::Vector3::Transform(offset, worldTransform_.matrix.world);
}

void Enemy::InitializeSerializableParm() {
	param_.LoadParams();

	life_      = static_cast<int>(param_.life);
	moveSpeed_ = param_.moveSpeed;
	killScore_ = static_cast<int>(param_.killScore);
	worldTransform_.scale = param_.scale;
}

void Enemy::DerivativeGui() {
	shooting_.ShowGui();
	param_.ShowGui();
}

/////////////////////////////////////////////////////////////////////////////////////////
//		EnemyParam
/////////////////////////////////////////////////////////////////////////////////////////
Enemy::EnemyParam::EnemyParam() {
	AddField("life", life).Category("Basic").Range(1.0f, 1000.0f);
	AddField("killScore", killScore).Category("Basic").Range(0.0f, 10000.0f);
	AddField("moveSpeed", moveSpeed).Category("Basic").Range(0.0f, 100.0f);
	AddField("scale", scale).Category("Basic");

	AddField("deathLength", death.length).Category("Death").Range(0.1f, 10.0f);
	AddField("deathRotateAxis", death.rotateAxis).Category("Death");

	AddField("collisionRadius", col.radius).Category("Collider").Range(0.1f, 50.0f);
}

CalyxEngine::ParamPath Enemy::EnemyParam::GetParamPath() const {
	return {CalyxEngine::ParamDomain::Game, "Enemy", "Actor/Enemy"};
}

void Enemy::Die() {
	// スコアを送る
	EnemyFactionActor::PublishKillScore();
}