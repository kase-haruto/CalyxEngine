#include "ProjectileObject.h"

#include <Engine/Objects/3D/Actor/Registry/SceneObjectRegistry.h>
#include <Engine/Scene/Context/SceneContext.h>

#include "Game\DemoPlayer\DemoPlayer.h"

REGISTER_SCENE_OBJECT(ProjectileObject)

ProjectileObject::ProjectileObject(
	const std::string&		   modelName,
	std::optional<std::string> objectName)
	: StageGimmickObjectBase(modelName, objectName) {
}

void ProjectileObject::OnCollisionEnter(Collider* other) {

	// ギミックなどへの干渉 or 相手側に追加する
	BaseGameObject* otherObj = other->GetOwner();
	bool isPlayerAttack = other->GetType() == ColliderType::Type_PlayerAttack;
	bool isPlayer = other->GetType() == ColliderType::Type_Player;
	if(otherObj && !isPlayerAttack && !isPlayer) {
		return;
	}
	// コライダーとモデルを無効化
	isHit_		= isPlayer;
	isParry_	= isPlayerAttack;
	if(isHit_) {
		isFlying_ = false;
	}
	targetTime_ = 0.0f;
	if(collider_) {
		collider_->SetCollisionEnabled(false);
	}
}

void ProjectileObject::ObjectInitialize() {

	BaseGameObject::InitializeCollider(ColliderKind::Box);
	if(collider_) {
		collider_->SetType(ColliderType::Type_EnemyAttack);
		collider_->SetTargetType(ColliderType::Type_Player | ColliderType::Type_PlayerAttack);
		collider_->SetOwner(this);
		collider_->SetCollisionEnabled(true);
	}
	isFlying_ = false;

	worldTransform_.inheritScale = false;
}

void ProjectileObject::ObjectUpdate(float dt) {

	if(worldTransform_.translation.y > param_.parryPositionY) {
		return;
	}

	if(!isFlying_) {
		if(targetTime_< 1.0f && isHit_) {
			targetTime_ += dt;
			if(targetTime_ >= 1.0f) {
				SetDrawEnable(false);
				velocity_ = CalyxEngine::Vector3::Zero();
			}
		}
	} else {
		if(param_.targetTime > targetTime_ && !isParry_) {
			targetTime_ += dt;
			auto player = SceneContext::Current()->FindObjectByName<DemoPlayer>("DemoPlayer");
			velocity_	= player->GetWorldTransform().GetWorldPosition() - GetWorldTransform().GetWorldPosition();
		}
		if(isParry_) {
			velocity_ = CalyxEngine::Vector3::Forward() + CalyxEngine::Vector3::Up();
		}
	}

	if(velocity_.Length() != 0.0f) {
		// 移動処理
		velocity_ = velocity_.Normalize() * param_.speed * dt;
		CalyxEngine::Vector3 prePos = worldTransform_.translation;
		worldTransform_.translation += velocity_;
		// 回転処理
		auto rotation = CalyxEngine::Quaternion::LookAt(
			worldTransform_.translation,
			prePos,
			CalyxEngine::Vector3::Up());
		worldTransform_.rotation = CalyxEngine::Quaternion::Slerp(
			worldTransform_.rotation, rotation, 0.2f);
	}
}
