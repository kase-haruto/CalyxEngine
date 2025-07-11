#include "CalyxHuman.h"

#include <Engine/Application/Input/Input.h>
#include <Engine/Foundation/Clock/ClockManager.h>

CalyxHuman::CalyxHuman(const std::string& modelName,
					   std::optional<std::string> objectName) :
	Actor::Actor(modelName, objectName){
	moveSpeed_ = 10.0f;

	//animationを追加
}

void CalyxHuman::Initialize(){}

void CalyxHuman::Update(){
	float dt = ClockManager::GetInstance()->GetDeltaTime();

	Move(dt);
	Turn();

	TransitionAnimation();

	BaseGameObject::Update();
}

void CalyxHuman::TransitionAnimation(){}

void CalyxHuman::Move(float dt){
	velocity_ = {0.0f, 0.0f, 0.0f};

	Vector2 leftStick = Input::GetInstance()->GetLeftStick();
	velocity_.x += leftStick.x;
	velocity_.z += leftStick.y;

	if (velocity_.Length() > 0.0f){
		velocity_.Normalize();
	}

	velocity_ *= moveSpeed_;

	// 移動加算
	worldTransform_.translation += velocity_ * dt;
}

void CalyxHuman::Turn(){
	Vector3 from = Vector3::Forward(); // (0, 0, 1)
	Vector3 to = velocity_.Normalize();                  // 移動方向

	if (to.Length() > 0.0f){
		Quaternion rot = Quaternion::FromToQuaternion(from, to);
		worldTransform_.rotation = rot;
	}
}
