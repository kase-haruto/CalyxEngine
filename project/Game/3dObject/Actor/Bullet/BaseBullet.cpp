#include "BaseBullet.h"
/* ========================================================================
/* include space
/* ===================================================================== */

/* engine */
/* external */
#include <externals/imgui/imgui.h>

BaseBullet::BaseBullet(const std::string& modelName, const std::string& name)
	: Actor::Actor(modelName, name) {

	moveSpeed_ = 50.0f;
	SetDrawEnable(false);

	// コライダーの形状をsphereに設定
	BaseGameObject::InitializeCollider(ColliderKind::Sphere);
}

/////////////////////////////////////////////////////////////////////////////////////////
//		初期化
/////////////////////////////////////////////////////////////////////////////////////////
void BaseBullet::ShootInitialize(const CalyxMath::Vector3& initPos, const CalyxMath::Vector3& velocity) {
	worldTransform_.translation = initPos;
	worldTransform_.scale		= {3, 3, 3};
	worldTransform_.Update(); // 行列を確定させる
	velocity_ = velocity;
	isAlive_  = true;
	OnShot();
}

/////////////////////////////////////////////////////////////////////////////////////////
//		更新
/////////////////////////////////////////////////////////////////////////////////////////
void BaseBullet::Update(float deltaTime) {
	// 通常移動とtrailFx_更新
	worldTransform_.translation += velocity_ * moveSpeed_ * deltaTime;

	// 寿命減少
	lifeTime_ -= deltaTime;

	if(lifeTime_ <= 0.0f) {
		isAlive_ = false;
	}
}

/////////////////////////////////////////////////////////////////////////////////////////
//		imgui
/////////////////////////////////////////////////////////////////////////////////////////
void BaseBullet::DerivativeGui() {}

void BaseBullet::OnCollisionEnter([[maybe_unused]] Collider* other) {
	isAlive_ = false;
}

void BaseBullet::OnShot() {}