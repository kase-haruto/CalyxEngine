#include "EnemyHomingBullet.h"

#include <Engine/Foundation/Clock/ClockManager.h>
#include <Engine/Scene/Utility/SceneUtility.h>

EnemyHomingBullet::EnemyHomingBullet(const std::string& modelName, const std::string& name)
	:BaseBullet::BaseBullet(modelName, name) {
	collider_->SetType(ColliderType::Type_EnemyAttack);
	collider_->SetTargetType(ColliderType::Type_Player);

	trailFx_ = SceneAPI::Instantiate<ParticleSystemObject>("playerBulletTrail");
	trailFx_->LoadConfig("Resources/Assets/Configs/Effect/HomingBulletTrail.json");

	shootFx_ = SceneAPI::Instantiate<ParticleSystemObject>("shootFx");
	shootFx_->LoadConfig("Resources/Assets/Configs/Effect/ShootFx.json");
}

EnemyHomingBullet::~EnemyHomingBullet() {
	auto ctx = SceneContext::Current();
	ctx->RemoveEditorObject(trailFx_);
	ctx->RemoveEditorObject(shootFx_);
}

void EnemyHomingBullet::ShootInitialize(const Vector3& initPos, const Vector3& velocity) {
	Vector3 initVel = (velocity.Length() > 0.001f)
		? velocity.Normalize() * homingSpeed_
		: Vector3(0, 0, 1) * homingSpeed_; // fallback

	BaseBullet::ShootInitialize(initPos, initVel);
}

void EnemyHomingBullet::Initialize() {
	auto self = shared_from_this();
	trailFx_->SetParent(self);
	shootFx_->SetParent(self);
}

void EnemyHomingBullet::OnShot() {}

void EnemyHomingBullet::SetTarget(const Actor* target) {
	target_ = target;
}

void EnemyHomingBullet::Update([[maybe_unused]] float dt) {
	if (target_ && target_->GetIsAlive()) {
		Vector3 objectOffset = { 0.0f,1.0f,0.0f };
		Matrix4x4 targetWorldMat = target_->GetWorldTransform().matrix.world;
		Vector3 centerPos = Vector3::Transform(objectOffset, targetWorldMat);

		Vector3 toTarget = centerPos - GetCenterPos();

		if (toTarget.Length() > 0.001f) {
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

const Vector3 EnemyHomingBullet::GetCenterPos()const {
	const Vector3 offset = { 0.0f, 1.0f, 0.0f };
	Vector3 worldPos = Vector3::Transform(offset, worldTransform_.matrix.world);
	return worldPos;
}