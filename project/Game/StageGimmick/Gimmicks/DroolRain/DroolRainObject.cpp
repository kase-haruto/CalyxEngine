#include "DroolRainObject.h"

#include <Engine/Objects/3D/Actor/Registry/SceneObjectRegistry.h>
#include <Engine\Foundation\Utility\Random\Random.h>
#include <Engine\Foundation\Utility\Ease\CxEase.h>

REGISTER_SCENE_OBJECT(DroolRainObject)

DroolRainObject::DroolRainObject(
	const std::string& modelName,
	std::optional<std::string> objectName)
	: StageGimmickObjectBase(modelName, objectName) {

	worldTransform_.translation.y += 2.5f;
}

void DroolRainObject::ObjectInitialize() {

	// コライダーの初期化
	BaseGameObject::InitializeCollider(ColliderKind::Sphere);
	if(collider_) {
		collider_->SetType(ColliderType::Type_EnemyAttack);
		collider_->SetTargetType(ColliderType::Type_Player);
		collider_->SetOwner(this);
		collider_->SetCollisionEnabled(true);
	}

	defaultScale_				 = worldTransform_.scale;
	worldTransform_.inheritScale = false;
	dropTime_					 = Random::Generate<float>(0.0f, 1.0f);
	isRaining_					 = false;
	isOnceSet_					 = true;
}

void DroolRainObject::ObjectUpdate(float dt) {

	if(isOnceSet_) {
		offsetY_ = worldTransform_.translation.y;
		isOnceSet_ = false;
		return;
	}

	if(dropTime_ >= 0.0f) {
		dropTime_ -= dt;
		if(dropTime_ < 0.0f) {
			isRaining_ = true;
			SetDrawEnable(true);
		}
	}

	if(!isRaining_) return;

	// 落下中の処理
	if(worldTransform_.GetWorldPosition().y >= 0.0f) {

		// よだれの落下処理
		float y = worldTransform_.translation.y;
		runtimeParam_.velocityY_ += runtimeParam_.accelerationY_ * dt;
		y += runtimeParam_.velocityY_ * dt;
		worldTransform_.translation.y = y;
		// よだれのスケール処理
		float scaleY = worldTransform_.scale.y;
		scaleY += runtimeParam_.airScaleSpeed_ * dt;
		worldTransform_.scale.y = scaleY;

	// 落下終了後の処理
	} else {
		// 地面についてから時間を測る
		runtimeParam_.groundLifeTime_ -= dt;
		float t = runtimeParam_.groundLifeTime_;
		if(t >= 0.0f) {

			// よだれの落下処理
			float y = worldTransform_.translation.y;
			y += runtimeParam_.groundVelocityY_ * dt;
			worldTransform_.translation.y = y;
			// よだれのスケール処理
			worldTransform_.scale += runtimeParam_.groundScaleSpeed_ * dt;
			worldTransform_.scale.y = defaultScale_.y * t;

		} else {
			worldTransform_.translation.y = offsetY_ * Random::Generate<float>(1.0f, 1.2f);
			worldTransform_.scale		  = defaultScale_;
			runtimeParam_				  = param_;
			dropTime_					  = Random::Generate<float>(0.0f, 1.0f);
			isRaining_					  = false;
			SetDrawEnable(false);
		}
	}
}
