#include "HomingBullet.h"

#include <Engine/Foundation/Clock/ClockManager.h>
#include <Engine/Scene/Utility/SceneUtility.h>
#include <Engine/Objects/Collider/BoxCollider.h>
#include <Engine\Objects\Collider\SphereCollider.h>

HomingBullet::HomingBullet(const std::string& modelName, const std::string& name) 
	:BaseBullet::BaseBullet(modelName, name){
	InitializeCollider(ColliderKind::Sphere);
	collider_->SetType(ColliderType::Type_PlayerAttack);
	collider_->SetTargetType(ColliderType::Type_Enemy);
	collider_->SetOwner(this);
	collider_->SetIsDrawCollider(false);
	auto* boxCollider = dynamic_cast<SphereCollider*>(collider_.get());
	boxCollider->SetRadius(1.5f);

	trailFx_ = SceneAPI::Instantiate<CalyxEffect::FxObject>("TrailFx");
	auto fx = trailFx_.lock();
	fx->LoadFromPath("Effect/HomingBulletTrail");
}

HomingBullet::~HomingBullet() = default;

void HomingBullet::ShootInitialize(const CalyxMath::Vector3& initPos, const CalyxMath::Vector3& velocity){
	CalyxMath::Vector3 initVel = (velocity.Length() > 0.001f)
		? velocity.Normalize() * homingSpeed_
		: CalyxMath::Vector3(0, 0, 1) * homingSpeed_; // fallback

	BaseBullet::ShootInitialize(initPos, initVel);


}

void HomingBullet::Initialize(){
	auto self = shared_from_this();
	auto fx = trailFx_.lock();
	fx->SetParent(self);
	fx->StopAll();
}

void HomingBullet::OnShot() {
	auto fx = trailFx_.lock();
	fx->PlayAll();
}

void HomingBullet::SetTarget(const Actor* target){
	target_ = target;
}

void HomingBullet::Update([[maybe_unused]]float dt){
	if (target_ && target_->GetIsAlive()){
		CalyxMath::Vector3 objectOffset = {0.0f,1.0f,0.0f};
		CalyxMath::Matrix4x4 targetWorldMat = target_->GetWorldTransform().matrix.world;
		CalyxMath::Vector3 centerPos = CalyxMath::Vector3::Transform(objectOffset, targetWorldMat);

		CalyxMath::Vector3 toTarget = centerPos - GetCenterPos();

		if (toTarget.Length() > 0.001f){
			CalyxMath::Vector3 desiredDir = toTarget.Normalize();
			CalyxMath::Vector3 currentDir = velocity_.Normalize();

			CalyxMath::Quaternion fromToQuat = CalyxMath::Quaternion::FromToQuaternion(currentDir, desiredDir);
			float t = std::clamp(rotateSpeed_ * dt, 0.0f, 1.0f);

			CalyxMath::Quaternion slerpedRot = CalyxMath::Quaternion::Slerp(CalyxMath::Quaternion::MakeIdentity(), fromToQuat, t);
			CalyxMath::Vector3 newDir = CalyxMath::Quaternion::RotateVector(currentDir, slerpedRot).Normalize();
			velocity_ = newDir * homingSpeed_;
		}
	}

	BaseBullet::Update(dt);
}

const CalyxMath::Vector3 HomingBullet::GetCenterPos()const{
	const CalyxMath::Vector3 offset = {0.0f, 1.0f, 0.0f};
	CalyxMath::Vector3 worldPos = CalyxMath::Vector3::Transform(offset, worldTransform_.matrix.world);
	return worldPos;
}