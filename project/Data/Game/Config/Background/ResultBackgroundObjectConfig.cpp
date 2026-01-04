#include "ResultBackgroundObjectConfig.h"

#include "Engine/System/Command/EditorCommand/GuiCommand/ImGuiHelper/GuiCmd.h"

ResultBackgroundObjectConfig::ResultBackgroundObjectConfig() {
	AddField("uvScrollSpeed",uvScrollSpeed);
	AddField("uvRotate",uvRotate);

	AddField("topPosition",topPosition);
	AddField("bottomPosition",bottomPosition);

	AddField("topRotate",topRotate);
	AddField("bottomRotate",bottomRotate);
}

CalyxEngine::ParamPath ResultBackgroundObjectConfig::GetParamPath() const { return {CalyxEngine::ParamDomain::Game,"ResultBackgroundObject"}; }

void ResultBackgroundObjectConfig::ShowGui() {
	GuiCmd::DragFloat("uvScrollSpeed.x",uvScrollSpeed);
	GuiCmd::DragFloat("uvRotate",uvRotate);
	GuiCmd::DragFloat2("topPosition",topPosition);
	GuiCmd::DragFloat2("bottomPosition",bottomPosition);
	GuiCmd::DragFloat("topRotate",topRotate);
	GuiCmd::DragFloat("bottomRotate",bottomRotate);
}