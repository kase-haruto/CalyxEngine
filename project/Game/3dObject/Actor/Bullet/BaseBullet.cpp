#include "BaseBullet.h"
/* ========================================================================
/* include space
/* ===================================================================== */

/* engine */
#include <Engine/Objects/Collider/BoxCollider.h>

/* external */
#include <externals/imgui/imgui.h>

BaseBullet::BaseBullet(const std::string& modelName, const std::string& name)
	:Actor::Actor(modelName, name){
	collider_->SetType(ColliderType::Type_PlayerAttack);
	collider_->SetTargetType(ColliderType::Type_Enemy);
	collider_->SetOwner(this);
	collider_->SetIsDrawCollider(false);
	auto* boxCollider = dynamic_cast< BoxCollider* >(collider_.get());
	boxCollider->SetSize(Vector3(3.0f, 3.0f, 3.0f));

	SetDrawEnable(false);
}

BaseBullet::~BaseBullet(){

}

/////////////////////////////////////////////////////////////////////////////////////////
//		初期化
/////////////////////////////////////////////////////////////////////////////////////////
void BaseBullet::ShootInitialize(const Vector3& initPos, const Vector3& velocity){
	worldTransform_.translation = initPos;
	velocity_ = velocity;
	moveSpeed_ = 22.0f;
	isAlive_ = true;
	OnShot();
}

/////////////////////////////////////////////////////////////////////////////////////////
//		更新
/////////////////////////////////////////////////////////////////////////////////////////
void BaseBullet::Update(float deltaTime){
	// 通常移動とtrailFx_更新
	worldTransform_.translation += velocity_ * moveSpeed_ * deltaTime;

	// 寿命減少
	lifeTime_ -= deltaTime;

	if (lifeTime_ <= 0.0f){
		isAlive_ = false;

	}
}

/////////////////////////////////////////////////////////////////////////////////////////
//		imgui
/////////////////////////////////////////////////////////////////////////////////////////
void BaseBullet::DerivativeGui(){

}

void BaseBullet::OnCollisionEnter([[maybe_unused]] Collider* other){

	if (other->GetType() == collider_->GetTargetType()){
		isAlive_ = false;
	}
}

void BaseBullet::OnShot(){}