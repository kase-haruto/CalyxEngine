#include "CameraTurnAroundAction.h"

// engine
#include "../3d/Camera3d.h"
#include "Engine/Foundation/Utility/Ease/CxEase.h"

#include <Engine/System/Command/EditorCommand/GuiCommand/ImGuiHelper/GuiCmd.h>
#include <Engine/Foundation/Utility/Func/CxUtils.h>

CameraTurnAroundAction::CameraTurnAroundAction() = default;

CameraTurnAroundAction::~CameraTurnAroundAction() = default;

void CameraTurnAroundAction::Execute() {
	if(turning_) return;

	turning_ = true;
	elapsed_ = 0.0f;
}

void CameraTurnAroundAction::Update(BaseCamera* cam, float dt) {
	if(!turning_ || !cam) return;

	elapsed_ += dt;
	float t	   = std::clamp(elapsed_ / turnTime_, 0.0f, 1.0f);
	float rate = Cx::Ease::ApplyEase(easeType_, t);

	WorldTransform& wt	   = cam->GetWorldTransform();
	wt.rotationSource  = RotationSource::Quaternion;
	Quaternion current = wt.rotation;

	Quaternion q180	  = Quaternion::MakeRotateY(Cx::Math::ToRadians(180.0f));
	Quaternion target = Quaternion::Multiply(q180, current);

	// 球面線形補間
	Quaternion newRot = Quaternion::Slerp(current, target, rate * dt * (1.0f / turnTime_));
	wt.rotation		  = newRot;

	if(t >= 1.0f) {
		turning_ = false;
	}
}

void CameraTurnAroundAction::ShowGui() {

	bool isOpen = true;
	ImGui::Begin(actionName_.c_str(), &isOpen);

	// パラメータ調整
	GuiCmd::DragFloat("turnTime", turnTime_);

	ImGui::End();
}