#include "RailCamera.h"

#include <Engine/Foundation/Utility/Func/MyFunc.h>
#include <Engine/Foundation/Utility/Func/MathFunc.h>
#include <Engine/Objects/3D/Actor/Registry/SceneObjectRegistry.h>
#include <Engine/Application/Input/Input.h>

// C++
#include <cmath>
#include <algorithm>

RailCamera::RailCamera() {}

RailCamera::RailCamera(const std::string& name){
	SceneObject::SetName(name, ObjectType::Camera);
}

void RailCamera::Initialize() {
	worldTransform_.Initialize();
	worldTransform_.translation = { 0.0f, 10.0f, 0.0f };

	BaseCamera::SetName("RailCamera");

	t_ = 0.0f;
	speed_ = 20.0f;
	tiltAngle_ = 0.3f;
	tiltLerpSpeed_ = 10.0f;
	targetTilt_ = 0.0f;
	zTiltOffset_ = 0.0f;
}

void RailCamera::Update(float deltaTime) {

	// ロール角を滑らかに補間
	zTiltOffset_ = std::lerp(zTiltOffset_, targetTilt_, tiltLerpSpeed_ * deltaTime);

	// Z軸方向に前進
	t_ += speed_ * deltaTime;

	Vector3 eye = Vector3(0.0f, 10.0f, t_);
	Vector3 forward = Vector3(0.0f, 0.0f, 1.0f);
	Vector3 target = eye + forward * 10.0f;

	Vector3 dir = (target - eye).Normalize();
	float horizontalDist = std::sqrt(dir.x * dir.x + dir.z * dir.z);
	worldTransform_.eulerRotation.x = std::atan2(-dir.y, horizontalDist);
	worldTransform_.eulerRotation.y = std::atan2(dir.x, dir.z);
	worldTransform_.eulerRotation.z = zTiltOffset_;

	// 位置更新
	worldTransform_.translation = eye;

}

void RailCamera::ShowGui() {
	worldTransform_.ShowImGui();
}

void RailCamera::AlwaysUpdate(float dt) {
	BaseCamera::AlwaysUpdate(dt);
}

Vector3 RailCamera::GetPosition() {
	return worldTransform_.GetWorldPosition();
}

REGISTER_SCENE_OBJECT(RailCamera)