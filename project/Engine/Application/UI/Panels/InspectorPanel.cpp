#include <Engine/Application/UI/Panels/InspectorPanel.h>
/* ========================================================================
/*  include Space
/* ===================================================================== */

// engine
#include <Engine/Editor/SceneObjectEditor.h>
#include <Engine/Objects/3D/Actor/SceneObject.h>
#include <Engine/Editor/BaseEditor.h>

// externals
#include <externals/imgui/imgui.h>

InspectorPanel::InspectorPanel()
	: IEngineUI("Inspector") {}

void InspectorPanel::Render() {
	ImGui::Begin(panelName_.c_str());

	if (selectedEditor_) {
		ImGui::Text("Editor: %s", selectedEditor_->GetEditorName().c_str());
		selectedEditor_->ShowImGuiInterface();
	} else if (selectedObject_){
		if (sceneObjectEditor_){
			sceneObjectEditor_->SetSceneObject(selectedObject_.get());
			sceneObjectEditor_->ShowImGuiInterface();
		}
	} else {
		ImGui::Text("Nothing is selected.");
	}

	ImGui::End();
}

void InspectorPanel::SetSelectedObject(const std::shared_ptr<SceneObject>&obj){
	selectedObject_ = obj;
	if (sceneObjectEditor_) sceneObjectEditor_->SetTarget(obj.get());
}


