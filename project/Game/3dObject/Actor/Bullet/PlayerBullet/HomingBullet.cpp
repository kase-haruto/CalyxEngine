#include "HomingBullet.h"

#include <Engine/Foundation/Clock/ClockManager.h>
#include <Engine/Scene/Utility/SceneUtility.h>
#include <Engine/Objects/Collider/BoxCollider.h>

HomingBullet::HomingBullet(const std::string& modelName, const std::string& name) 
	:BaseBullet::BaseBullet(modelName, name){
	collider_->SetType(ColliderType::Type_PlayerAttack);
	collider_->SetTargetType(ColliderType::Type_Enemy);
	collider_->SetOwner(this);
	collider_->SetIsDrawCollider(false);
	auto* boxCollider = dynamic_cast<BoxCollider*>(collider_.get());
	boxCollider->SetSize(Vector3(3.0f, 3.0f, 3.0f));


	trailFx_ = SceneAPI::Instantiate<ParticleSystemObject>("playerBulletTrail");
	trailFx_->LoadConfig("Effect/HomingBulletTrail");

	shootFx_ = SceneAPI::Instantiate<ParticleSystemObject>("shootFx");
	shootFx_->LoadConfig("Effect/ShootFx");
}

HomingBullet::~HomingBullet() = default;

void HomingBullet::ShootInitialize(const Vector3& initPos, const Vector3& velocity){
	Vector3 initVel = (velocity.Length() > 0.001f)
		? velocity.Normalize() * homingSpeed_
		: Vector3(0, 0, 1) * homingSpeed_; // fallback

	BaseBullet::ShootInitialize(initPos, initVel);
}

void HomingBullet::Initialize(){
	auto self = shared_from_this();
	trailFx_->SetParent(self);
	shootFx_->SetParent(self);
}

void HomingBullet::OnShot() {
}

void HomingBullet::SetTarget(const Actor* target){
	target_ = target;
}

void HomingBullet::Update([[maybe_unused]]float dt){
	if (target_ && target_->GetIsAlive()){
		Vector3 objectOffset = {0.0f,1.0f,0.0f};
		Matrix4x4 targetWorldMat = target_->GetWorldTransform().matrix.world;
		Vector3 centerPos = Vector3::Transform(objectOffset, targetWorldMat);

		Vector3 toTarget = centerPos - GetCenterPos();

		if (toTarget.Length() > 0.001f){
			Vector3 desiredDir = toTarget.Normalize();
			Vector3 currentDir = velocity_.Normalize();

			Quaternion fromToQuat = Quaternion::FromToQuaternion(currentDir, desiredDir);
			float t = std::clamp(rotateSpeed_ * dt, 0.0f, 1.0f);

			Quaternion slerpedRot = Quaternion::Slerp(Quaternion::MakeIdentity(), fromToQuat, t);
			Vector3 newDir = Quaternion::RotateVector(currentDir, slerpedRot).Normalize();
			velocity_ = newDir * homingSpeed_;
		}
	}

	BaseBullet::Update(dt);
}

const Vector3 HomingBullet::GetCenterPos()const{
	const Vector3 offset = {0.0f, 1.0f, 0.0f};
	Vector3 worldPos = Vector3::Transform(offset, worldTransform_.matrix.world);
	return worldPos;
}