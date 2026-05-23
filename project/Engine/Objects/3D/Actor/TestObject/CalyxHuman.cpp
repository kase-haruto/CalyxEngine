#include "CalyxHuman.h"

#include <Engine/Foundation/Input/Input.h>
#include <Engine/Foundation/Clock/ClockManager.h>
#include <Engine/Foundation/Utility/FileSystem/ConfigPathResolver/ConfigPathResolver.h>
#include <Engine/Objects/3D/Actor/Registry/SceneObjectRegistry.h>

CalyxHuman::CalyxHuman(const std::string& modelName,
					   std::optional<std::string> objectName) :
	Actor::Actor(modelName, objectName){
	moveSpeed_ = 10.0f;
	//animationを追加
	//GetAnimationModel()->AddAnimation("idle", "idle.gltf");

}

CalyxHuman::CalyxHuman(){
	moveSpeed_ = 10.0f;
	//animationを追加
//	GetCalyxEngine::AnimationModel()->AddAnimation("idle", "idle.gltf");

}


void CalyxHuman::Initialize(){
}

void CalyxHuman::Update(float dt){
	Move(dt);
	Turn();

	TransitionAnimation();
}
std::optional<CalyxEngine::Vector3> CalyxHuman::GetJointWorldPos(const std::string& name) const {
	const CalyxEngine::AnimationModel* anim = AnimationModel();
	if (!anim) return std::nullopt;

	auto matOpt = anim->GetJointMatrix(name);
	if (!matOpt) return std::nullopt;

	CalyxEngine::Matrix4x4 worldM = (*matOpt) * worldTransform_.matrix.world;

	CalyxEngine::Vector3 pos = {
		worldM.m[3][0],
		worldM.m[3][1],
		worldM.m[3][2]
	};
	return pos;
}

void CalyxHuman::TransitionAnimation(){
	CalyxEngine::Vector2 stickInput = CalyxFoundation::Input::GetInstance()->GetLeftStick();
	bool isMoving = stickInput.Length() > 0.1f;
	auto* model = AnimationModel();

	if (isMoving){
		if (model->GetCurrentAnimationName() != "run"){
			model->PlayAnimation("run", 0.2f);
		}
	} else{
		if (model->GetCurrentAnimationName() != "idle"){
			model->PlayAnimation("idle", 0.2f);
		}
	}
}

void CalyxHuman::Move(float dt){
	velocity_ = {0.0f, 0.0f, 0.0f};

	CalyxEngine::Vector2 leftStick = CalyxFoundation::Input::GetInstance()->GetLeftStick();
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
	CalyxEngine::Vector3 from = CalyxEngine::Vector3::Forward(); // (0, 0, 1)
	CalyxEngine::Vector3 to = velocity_.Normalize();                  // 移動方向

	if (to.Length() > 0.0f){
		CalyxEngine::Quaternion rot = CalyxEngine::Quaternion::FromToQuaternion(from, to);
		worldTransform_.rotation = rot;
	}
}

REGISTER_SCENE_OBJECT(CalyxHuman)