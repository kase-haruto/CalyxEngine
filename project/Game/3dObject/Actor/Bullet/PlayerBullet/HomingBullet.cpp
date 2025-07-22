#include "HomingBullet.h"

#include <Engine/Foundation/Clock/ClockManager.h>

HomingBullet::HomingBullet(const std::string& modelName, const std::string& name) 
	:BaseBullet::BaseBullet(modelName, name){
	collider_->SetType(ColliderType::Type_PlayerAttack);
	collider_->SetTargetType(ColliderType::Type_Enemy);
}

void HomingBullet::SetTarget(const WorldTransform* target){
	target_ = target;
}

void HomingBullet::Update(){
	if (target_){
		Vector3 toTarget = target_->GetWorldPosition() - GetWorldPosition();

		if (toTarget.Length() > 0.001f){
			Vector3 desiredDir = toTarget.Normalize();
			Vector3 currentDir = velocity_.Normalize();

			float dt = ClockManager::GetInstance()->GetDeltaTime();
			float t = std::clamp(rotateSpeed_ * dt, 0.0f, 1.0f);

			Vector3 newDir = Vector3::Lerp(currentDir, desiredDir, t).Normalize();
			velocity_ = newDir * homingSpeed_;
		}
	}

	BaseBullet::Update();
}