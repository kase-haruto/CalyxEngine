#include "FireLoadObject.h"

#include <Engine/Objects/3D/Actor/Registry/SceneObjectRegistry.h>
#include <Engine/Foundation/Math/MathUtil.h>
#include <Engine\Foundation\Utility\Ease\CxEase.h>
#include <cmath>

REGISTER_SCENE_OBJECT(FireLoadObject)

FireLoadObject::FireLoadObject(
	const std::string&		   modelName,
	std::optional<std::string> objectName)
	: StageGimmickObjectBase(modelName, objectName) {
}

void FireLoadObject::BurnActive() {
	SetIsBurn(true);
	SetIsIgnition(true);
	if(collider_) {
		collider_->SetCollisionEnabled(true);
	}
	if(model_) {
		BaseGameObject::SetDrawEnable(true);
	}
}

void FireLoadObject::OnCollisionEnter(Collider* other) {
	// ギミックなどへの干渉 or 相手側に追加する
	BaseGameObject* otherObj = other->GetOwner();
	if(otherObj && other->GetType() != ColliderType::Type_PlayerAttack) {
		return;
	}
	// コライダーとモデルを無効化
	isBurn_ = false;
	if(collider_) {
		collider_->SetCollisionEnabled(false);
	}
	if(model_) {
		BaseGameObject::SetDrawEnable(false);
	}
}

void FireLoadObject::ObjectInitialize() {

	// コライダーの初期化
	BaseGameObject::InitializeCollider(ColliderKind::Box);
	if(collider_) {
		collider_->SetType(ColliderType::Type_EnemyAttack);
		collider_->SetTargetType(ColliderType::Type_Player | ColliderType::Type_PlayerAttack);
		collider_->SetOwner(this);
		collider_->SetCollisionEnabled(false);
	}

	defaultScale_ = {1.0f, 1.0f, 1.0f};
}

void FireLoadObject::ObjectUpdate(float dt) {

	if(!isBurn_) {
		worldTransform_.scale = {0.0f, 0.0f, 0.0f};
		if(model_ && isDrawEnable_) {
			BaseGameObject::SetDrawEnable(false);
		}
		return;
	};
	IgnitionUpdate(dt);

}

void FireLoadObject::IgnitionUpdate(float dt) {
	if(!isIgnition_) return;

	ignitionTime_ += dt;
	float t = ignitionTime_ / (std::max)(0.001f, param_.time);
	if(t >= 1.0f) {
		t				 = 1.0f;
		isIgnition_	 = false;
		 ignitionTime_ = 0.0f;

	}
	 const float peakT = 0.3f;
	if(t < peakT) {
		float normT			   = t / peakT;
		float easeT			   = CalyxEngine::ApplyEase(CalyxEngine::EaseType::EaseInCubic, normT);
		worldTransform_.scale.y = easeT * param_.burnHeight;
		worldTransform_.scale.x = easeT * (defaultScale_.x * 1.2f);
		worldTransform_.scale.z = easeT * (defaultScale_.z * 1.2f);
	}
	else {
		float normT			   = (t - peakT) / (1.0f - peakT);
		float easeT			   = CalyxEngine::ApplyEase(CalyxEngine::EaseType::EaseInOutSine, normT);
		worldTransform_.scale.y = CalyxEngine::Lerp(param_.burnHeight, defaultScale_.y, easeT);
		worldTransform_.scale.x = CalyxEngine::Lerp(defaultScale_.x * 1.2f, defaultScale_.x, easeT);
		worldTransform_.scale.z = CalyxEngine::Lerp(defaultScale_.z * 1.2f, defaultScale_.z, easeT);
	}
}
