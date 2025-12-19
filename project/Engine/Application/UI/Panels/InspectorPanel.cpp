#include "InspectorPanel.h"

// engine
#include <Engine/Editor/SceneObjectEditor.h>
#include <Engine/Editor/BaseEditor.h>

// externals
#include <externals/imgui/imgui.h>

namespace CalyxEditor {
	/////////////////////////////////////////////////////////////////////////////////////////
	//		コンストラクタ
	/////////////////////////////////////////////////////////////////////////////////////////
	InspectorPanel::InspectorPanel()
		: IEngineUI("Inspector") {}

	/////////////////////////////////////////////////////////////////////////////////////////
	//		imgui描画
	/////////////////////////////////////////////////////////////////////////////////////////
	void InspectorPanel::Render() {
		bool open = true;
		ImGui::Begin(panelName_.c_str(), &open);

		// --- Editor が選択されている場合 ---
		if(selectedEditor_) {
			ImGui::Text("Editor: %s", selectedEditor_->GetEditorName().c_str());
			selectedEditor_->ShowImGuiInterface();
		} else {
			auto sp = selectedObject_.lock();
			if(sp && sceneObjectEditor_) {
				sceneObjectEditor_->SetSceneObject(sp.get());
				sceneObjectEditor_->ShowImGuiInterface();
			} else {
				ImGui::Text("Nothing is selected.");
			}
		}

		ImGui::End();
		if(!open) SetShow(false);
	}

	/////////////////////////////////////////////////////////////////////////////////////////
	//		エディタセット
	/////////////////////////////////////////////////////////////////////////////////////////
	void InspectorPanel::SetSelectedEditor(BaseEditor* editor) {
		selectedEditor_ = editor;
		selectedObject_.reset();
	}

	/////////////////////////////////////////////////////////////////////////////////////////
	//		オブジェクトセット
	/////////////////////////////////////////////////////////////////////////////////////////
	void InspectorPanel::SetSelectedObject(std::weak_ptr<SceneObject> obj) {
		selectedObject_ = obj;
		selectedEditor_ = nullptr;

		if(sceneObjectEditor_) {
			if(auto sp = obj.lock()) {
				sceneObjectEditor_->SetSceneObject(sp.get());
			} else {
				sceneObjectEditor_->SetSceneObject(nullptr);
			}
		}
	}
} // namespace CalyxEditor

