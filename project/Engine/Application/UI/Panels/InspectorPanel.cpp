#include "InspectorPanel.h"

// engine
#include <Engine/Editor/SceneObjectEditor.h>
#include <Engine/Editor/BaseEditor.h>

// externals
#include "Engine/System/Command/EditorCommand/GuiCommand/ImGuiHelper/GuiCmd.h"

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
		if(!IsShow()) return;

		// タブの初期化
		if (tabs_.empty()) {
			// 将来的にはアイコン画像を使用するが、現在は仮置き
			tabs_ = {
				{"All",           "Textures/white1x1.png", "All"},
				{"Tf",            "Textures/white1x1.png", "Tf"},
				{"Material",      "Textures/white1x1.png", "Material"},
				{"ParameterData", "Textures/white1x1.png", "ParameterData"},
				{"Draw",          "Textures/white1x1.png", "Draw"},
				{"Collider",      "Textures/white1x1.png", "Collider"},
			};
		}

		bool open = true;
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f)); // サイドバーは全高使用
		if(ImGui::Begin(panelName_.c_str(), &open)) {

			if(selectedEditor_) { 
				ImGui::Text("Editor: %s", selectedEditor_->GetEditorName().c_str());
				ImGui::Separator();
				selectedEditor_->ShowImGuiInterface();
			} 
			else {
				auto sp = selectedObject_.lock();
				if(sp && sceneObjectEditor_) {
					
					// レイアウト: サイドバー | コンテンツ
					if (ImGui::BeginTable("InspectorMainLayout", 2, ImGuiTableFlags_Resizable | ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_SizingFixedFit)) {
						
						ImGui::TableSetupColumn("Sidebar", ImGuiTableColumnFlags_WidthFixed, 40.0f);
						ImGui::TableSetupColumn("Content", ImGuiTableColumnFlags_None);
						
						ImGui::TableNextColumn();
						RenderSidebar();

						ImGui::TableNextColumn();
						
						// コンテンツのスクロール領域開始
						if(ImGui::BeginChild("##ContentScroll", ImVec2(0, 0), false, ImGuiWindowFlags_None)) {
						
							ImGui::Dummy(ImVec2(0, 4));
							ImGui::Indent(4.0f);
							
							ImGui::TextDisabled("Type: %s", sp->GetObjectTypeName().c_str());
							ImGui::SameLine();
							ImGui::Text("%s", sp->GetName().c_str());
							ImGui::Separator();
							ImGui::Spacing();

							sceneObjectEditor_->SetSceneObject(sp.get());

							// フィルタ設定
							const auto& activeTab = tabs_[currentTabIndex_];
							GuiCmd::SetSectionFilter(activeTab.filterSection.c_str());

							sceneObjectEditor_->ShowImGuiInterface();
							
							ImGui::Unindent(4.0f);
						}
						ImGui::EndChild();

						ImGui::EndTable();
					}

				} else {
					ImGui::Text("Nothing is selected.");
				}
			}
		}
		ImGui::End();
		ImGui::PopStyleVar();

		if(!open) SetShow(false);
	}

	void InspectorPanel::RenderSidebar() {
		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0,0,0,0)); // 背景透明
		
		for (int i = 0; i < tabs_.size(); ++i) {
			const auto& tab = tabs_[i];
			bool isSelected = (currentTabIndex_ == i);
			
			// 選択中の場合ハイライト
			if (isSelected) {
				ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.2f, 0.2f, 1.0f));
			}

			ImGui::PushID(i);
			
			// テクスチャロードが未実装のため、ボタンで代用（先頭2文字表示）
			if (ImGui::Button(tab.name.substr(0, 2).c_str(), ImVec2(32, 32))) {
				currentTabIndex_ = i;
			}
			
			if (ImGui::IsItemHovered()) {
				ImGui::SetTooltip("%s", tab.name.c_str());
			}

			if (isSelected) {
				ImGui::PopStyleColor();
			}
			ImGui::PopID();
			
			ImGui::Spacing();
		}
		
		ImGui::PopStyleColor();
	}

	void InspectorPanel::RenderContent() {
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

