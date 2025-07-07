#include "RailCamera.h"

#include <Engine/Foundation/Utility/Func/MyFunc.h>
#include <Engine/Foundation/Clock/ClockManager.h>
#include <Engine/Foundation/Utility/Func/MathFunc.h>
#include <Engine/Application/Input/Input.h>

// C++
#include <cmath>
#include <algorithm>

RailCamera::RailCamera() {}

void RailCamera::Initialize() {
	transform_.translate = { 0.0f, 10.0f, 0.0f };
	transform_.scale = { 1.0f, 1.0f, 1.0f };
	transform_.rotate = { 0.0f, 0.0f, 0.0f };

	worldTransform_.Initialize();
	worldTransform_.scale = transform_.scale;
	worldTransform_.eulerRotation = transform_.rotate;
	worldTransform_.translation = transform_.translate;

	BaseCamera::SetName("RailCamera");

	t_ = 0.0f;
	speed_ = 0.0f;
	tiltAngle_ = 0.3f;
	tiltLerpSpeed_ = 10.0f;
	targetTilt_ = 0.0f;
	zTiltOffset_ = 0.0f;
}

void RailCamera::Update() {
	float deltaTime = ClockManager::GetInstance()->GetDeltaTime();

	// ロール角を滑らかに補間
	zTiltOffset_ = std::lerp(zTiltOffset_, targetTilt_, tiltLerpSpeed_ * deltaTime);

	// Z軸方向に前進
	t_ += speed_ * deltaTime;

	Vector3 eye = Vector3(0.0f, 10.0f, t_);
	Vector3 forward = Vector3(0.0f, 0.0f, 1.0f);
	Vector3 target = eye + forward * 10.0f; // 10ユニット先を見る

	// カメラ回転（ピッチ・ヨー・ロール）
	Vector3 dir = (target - eye).Normalize();
	float horizontalDist = std::sqrt(dir.x * dir.x + dir.z * dir.z);
	transform_.rotate.x = std::atan2(-dir.y, horizontalDist);
	transform_.rotate.y = std::atan2(dir.x, dir.z);
	transform_.rotate.z = zTiltOffset_;

	// 位置更新
	transform_.translate = eye;

	worldTransform_.scale = transform_.scale;
	worldTransform_.eulerRotation = transform_.rotate;
	worldTransform_.translation = transform_.translate;

	BaseCamera::Update();
	worldTransform_.Update(viewProjectionMatrix_);
}

Vector3 RailCamera::GetPosition() {
	return transform_.translate;
}
