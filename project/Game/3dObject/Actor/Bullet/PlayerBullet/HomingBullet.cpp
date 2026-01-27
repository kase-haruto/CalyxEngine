#include "HomingBullet.h"

#include <Engine/Foundation/Clock/ClockManager.h>
#include <Engine/Scene/Utility/SceneUtility.h>
#include <Engine/Objects/Collider/BoxCollider.h>
#include <Engine\Objects\Collider\SphereCollider.h>

#include <algorithm>
#include <cmath>

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

	param_.LoadParams();

	homingSpeed_ = param_.homingSpeed;
	rotateSpeed_ = param_.rotateSpeed;
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

void HomingBullet::SetSpeedEase(float startSpeed, float endSpeed, float duration, CalyxEase::EaseType type) {
	startSpeed_    = startSpeed;
	endSpeed_      = endSpeed;
	easeDuration_  = duration;
	easeTimer_     = 0.0f;
	isSpeedEasing_ = true;
	easeType_      = type;

	// 現在のhomingSpeed_も更新しておく
	homingSpeed_ = startSpeed;

	// 初速設定（すでにShootInitializeが呼ばれている場合の対応）
	if (velocity_.LengthSquared() > 0.0001f) {
		velocity_ = velocity_.Normalize() * startSpeed_;
	}
}

void HomingBullet::Update([[maybe_unused]]float dt){

	// 速度イージング
	if (isSpeedEasing_) {
		easeTimer_ += dt;
		float ratio = easeDuration_ > 0.0f ? easeTimer_ / easeDuration_ : 1.0f;

		float currentSpeed = CalyxEase::EaseLerp(startSpeed_, endSpeed_, ratio, easeType_);

		// 速度更新
		homingSpeed_ = currentSpeed;
		if (velocity_.LengthSquared() > 0.0001f) {
			velocity_ = velocity_.Normalize() * currentSpeed;
		}
	}

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

/////////////////////////////////////////////////////////////////////////////////////////
//		BulletParam
/////////////////////////////////////////////////////////////////////////////////////////
HomingBullet::BulletParam::BulletParam() {
	AddField("homingSpeed", homingSpeed).Category("Basic").Range(0.1f, 500.0f);
	AddField("rotateSpeed", rotateSpeed).Category("Basic").Range(0.1f, 1000.0f);
}

CalyxEngine::ParamPath HomingBullet::BulletParam::GetParamPath() const {
	return {CalyxEngine::ParamDomain::Game, "HomingBullet", "Actor/Bullet/Player"};
}