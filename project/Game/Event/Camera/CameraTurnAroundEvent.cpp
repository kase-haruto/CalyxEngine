#include "CameraTurnAroundEvent.h"
/* ========================================================================
/*		include space
/* ===================================================================== */
#include "Engine/Foundation/Utility/Ease/CxEase.h"

#include <Engine/Graphics/Camera/Manager/CameraManager.h>
#include <Engine/System/Command/EditorCommand/GuiCommand/ImGuiHelper/GuiCmd.h>

/////////////////////////////////////////////////////////////////////////////////////////
//		ctor / dtor
/////////////////////////////////////////////////////////////////////////////////////////
CameraTurnAroundEvent::CameraTurnAroundEvent() {
	// 操作対象カメラにメインカメラをセット
	cam_ = CameraManager::GetMain3d();
	SceneObject::SetName("CameraTurnAroundEvent",ObjectType::GameObject);
}

CameraTurnAroundEvent::CameraTurnAroundEvent(const std::string& name): CameraEventObject(name) {
	// 操作対象カメラにメインカメラをセット
	cam_ = CameraManager::GetMain3d();
}

CameraTurnAroundEvent::~CameraTurnAroundEvent() = default;

/////////////////////////////////////////////////////////////////////////////////////////
//		発火時処理
/////////////////////////////////////////////////////////////////////////////////////////
void CameraTurnAroundEvent::OnCollisionEnter([[maybe_unused]] Collider* other) {
	// カメラを振り向かせる
}

void CameraTurnAroundEvent::OnCollisionExit([[maybe_unused]] Collider* other) {
	// カメラを戻す
}

void CameraTurnAroundEvent::DerivativeGui() {
	if(ImGui::CollapsingHeader("Turn Around Parm")) {
		GuiCmd::DragFloat3("dir",direction_);
		GuiCmd::DragFloat("time",time_);
		Cx::Ease::SelectEase(easeType_);
	}
}