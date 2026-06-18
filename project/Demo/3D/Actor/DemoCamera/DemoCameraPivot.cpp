#include "DemoCameraPivot.h"

// engine
#include "Engine/System/Command/EditorCommand/GuiCommand/ImGuiHelper/GuiCmd.h"
#include "UI/Panels/InspectorPanel.h"
#include <Engine/Objects/3D/Actor/Registry/SceneObjectRegistry.h>

REGISTER_SCENE_OBJECT(DemoCameraPivot)

DemoCameraPivot::DemoCameraPivot() {}


void DemoCameraPivot::ShowGui() {
	// --- トランスフォーム ---
	if(GuiCmd::BeginSection(CalyxEngine::ParamFilterSection::Object)) {
		worldTransform_.ShowImGui("world");
		GuiCmd::EndSection();
	}
}