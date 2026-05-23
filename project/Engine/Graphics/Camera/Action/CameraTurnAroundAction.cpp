#include "CameraTurnAroundAction.h"

#include <Engine/Foundation/Utility/Ease/CxEase.h>
#include <Engine/Graphics/Camera/Manager/CameraManager.h>
#include <Engine/System/Command/EditorCommand/GuiCommand/ImGuiHelper/GuiCmd.h>

CameraTurnAroundAction::CameraTurnAroundAction() = default;
CameraTurnAroundAction::~CameraTurnAroundAction() = default;

void CameraTurnAroundAction::Execute() {
	if (turning_) return;

	Camera3d* cam = CameraManager::GetMain3d();
	if (!cam) return;

	WorldTransform& wt = cam->GetWorldTransform();
	startRot_ = wt.rotation;

	// 目標回転を direction_ 方向に向かせる
	CalyxEngine::Vector3 eye = wt.translation;
	CalyxEngine::Vector3 target = eye + direction_.Normalize();
	targetRot_ = CalyxEngine::Quaternion::LookAt(eye, target);

	elapsed_ = 0.0f;
	turning_ = true;
}

void CameraTurnAroundAction::Update(BaseCamera* cam, float dt) {
	if (!turning_ || !cam) return;

	elapsed_ += dt;
	float t = std::clamp(elapsed_ / turnTime_, 0.0f, 1.0f);
	float easeT = CalyxEngine::ApplyEase(easeType_, t);

	CalyxEngine::Quaternion newRot = CalyxEngine::Quaternion::Slerp(startRot_, targetRot_, easeT);
	cam->GetWorldTransform().rotation = newRot;
	cam->GetWorldTransform().rotationSource = RotationSource::Quaternion;

	if (t >= 1.0f) turning_ = false;
}

void CameraTurnAroundAction::ShowGui() {
	bool isOpen = true;
	ImGui::Begin(actionName_.c_str(), &isOpen);

	GuiCmd::DragFloat("Turn Time", turnTime_);
	ImGui::DragFloat3("Direction", &direction_.x, 0.1f);

	ImGui::End();
}
