#include "CalyxHuman.h"

#include <Engine/Application/Input/Input.h>
#include <Engine/Foundation/Clock/ClockManager.h>
#include <Engine/Application/Effects/Intermediary/FxIntermediary.h>
#include <Engine/Foundation/Utility/FileSystem/ConfigPathResolver/ConfigPathResolver.h>
#include <Engine/Objects/3D/Actor/Registry/SceneObjectRegistry.h>

CalyxHuman::CalyxHuman(const std::string& modelName,
					   std::optional<std::string> objectName) :
	Actor::Actor(modelName, objectName){
	moveSpeed_ = 10.0f;
	//animationを追加
	GetAnimationModel()->AddAnimation("idle", "idle.gltf");

	trailFx_ = std::make_shared<ParticleSystemObject>("playerBulletTrail");
	trailFx_->LoadConfig("Resources/Assets/Configs/Effect/runFx.json");
	FxIntermediary::GetInstance()->Attach(trailFx_);

}

CalyxHuman::CalyxHuman(){
	moveSpeed_ = 10.0f;
	//animationを追加
//	GetAnimationModel()->AddAnimation("idle", "idle.gltf");

	trailFx_ = std::make_shared<ParticleSystemObject>("playerBulletTrail");
	trailFx_->LoadConfig("Resources/Assets/Configs/Effect/runFx.json");
	FxIntermediary::GetInstance()->Attach(trailFx_);
}


void CalyxHuman::Initialize(){
	auto self = shared_from_this();
	trailFx_->SetParent(self);
}

void CalyxHuman::Update(){
	float dt = ClockManager::GetInstance()->GetDeltaTime();

	Move(dt);
	Turn();

	if (GetJointWorldPos("RightHandIndex1").has_value()) {
		trailFx_->position_ = GetJointWorldPos("mixamorig:RightHandIndex1").value();
	}

	TransitionAnimation();

	if (Input::GetInstance()->PushGamepadButton(PAD_BUTTON::A)) {
		trailFx_->Play();
	}

	collider_->SetCollisionEnabled(false);

	BaseGameObject::Update();
}

std::optional<Vector3> CalyxHuman::GetJointWorldPos(const std::string& name) const {
	const AnimationModel* anim = GetAnimationModel();
	if (!anim) return std::nullopt;

	auto matOpt = anim->GetJointMatrix(name);
	if (!matOpt) return std::nullopt;

	Matrix4x4 worldM = (*matOpt) * worldTransform_.matrix.world;

	Vector3 pos = {
		worldM.m[3][0],
		worldM.m[3][1],
		worldM.m[3][2]
	};
	return pos;
}

void CalyxHuman::TransitionAnimation(){
	Vector2 stickInput = Input::GetInstance()->GetLeftStick();
	bool isMoving = stickInput.Length() > 0.1f;
	auto* model = GetAnimationModel();

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

REGISTER_SCENE_OBJECT(CalyxHuman)