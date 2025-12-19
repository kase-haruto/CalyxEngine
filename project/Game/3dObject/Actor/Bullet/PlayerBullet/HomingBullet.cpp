#include "HomingBullet.h"

#include <Engine/Foundation/Clock/ClockManager.h>
#include <Engine/Scene/Utility/SceneUtility.h>
#include <Engine/Objects/Collider/BoxCollider.h>

HomingBullet::HomingBullet(const std::string& modelName, const std::string& name) 
	:BaseBullet::BaseBullet(modelName, name){
	InitializeCollider(ColliderKind::Sphere);
	collider_->SetType(ColliderType::Type_PlayerAttack);
	collider_->SetTargetType(ColliderType::Type_Enemy);
	collider_->SetOwner(this);
	collider_->SetIsDrawCollider(false);
	auto* boxCollider = dynamic_cast<SphereCollider*>(collider_.get());
	boxCollider->SetRadius(1.5f);

	trailFx_ = SceneAPI::Instantiate<FxObject>("TrailFx");
	auto fx = trailFx_.lock();
	fx->LoadFromPath("Effect/HomingBulletTrail");
}

HomingBullet::~HomingBullet() = default;

void HomingBullet::ShootInitialize(const CxMath::Vector3& initPos, const CxMath::Vector3& velocity){
	CxMath::Vector3 initVel = (velocity.Length() > 0.001f)
		? velocity.Normalize() * homingSpeed_
		: CxMath::Vector3(0, 0, 1) * homingSpeed_; // fallback

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
		CxMath::Vector3 objectOffset = {0.0f,1.0f,0.0f};
		CxMath::Matrix4x4 targetWorldMat = target_->GetWorldTransform().matrix.world;
		CxMath::Vector3 centerPos = CxMath::Vector3::Transform(objectOffset, targetWorldMat);

		CxMath::Vector3 toTarget = centerPos - GetCenterPos();

		if (toTarget.Length() > 0.001f){
			CxMath::Vector3 desiredDir = toTarget.Normalize();
			CxMath::Vector3 currentDir = velocity_.Normalize();

			CxMath::Quaternion fromToQuat = CxMath::Quaternion::FromToQuaternion(currentDir, desiredDir);
			float t = std::clamp(rotateSpeed_ * dt, 0.0f, 1.0f);

			CxMath::Quaternion slerpedRot = CxMath::Quaternion::Slerp(CxMath::Quaternion::MakeIdentity(), fromToQuat, t);
			CxMath::Vector3 newDir = CxMath::Quaternion::RotateVector(currentDir, slerpedRot).Normalize();
			velocity_ = newDir * homingSpeed_;
		}
	}

	BaseBullet::Update(dt);
}

const CxMath::Vector3 HomingBullet::GetCenterPos()const{
	const CxMath::Vector3 offset = {0.0f, 1.0f, 0.0f};
	CxMath::Vector3 worldPos = CxMath::Vector3::Transform(offset, worldTransform_.matrix.world);
	return worldPos;
}