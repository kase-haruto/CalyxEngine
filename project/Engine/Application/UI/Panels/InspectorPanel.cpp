#include "InspectorPanel.h"

// engine
#include <Engine/Editor/SceneObjectEditor.h>
#include <Engine/Editor/BaseEditor.h>
#include <Engine/Assets/Texture/TextureManager.h>
#include <Engine/Foundation/Utility/Converter/EnumConverter.h>

// externals
#include "Engine/System/Command/EditorCommand/GuiCommand/ImGuiHelper/GuiCmd.h"
#include <externals/imgui/imgui.h>

namespace CalyxEditor {
	/////////////////////////////////////////////////////////////////////////////////////////
	//		コンストラクタ
	/////////////////////////////////////////////////////////////////////////////////////////
	InspectorPanel::InspectorPanel()
		: IEngineUI("Inspector") {}

	// ============================================================================
	//		imgui描画
	// ============================================================================
	void InspectorPanel::Render() {
		if(!IsShow()) return;

		// タブの初期化（マスターを保持）
		if(allTabs_.empty()) {
			auto& tm = *TextureManager::GetInstance();
			allTabs_ = {
					{rootPath_ + "inspectorUI_Al.dds",ParamFilterSection::All},
					{rootPath_ + "inspectorUI_Ob.dds",ParamFilterSection::Object},
					{rootPath_ + "inspectorUI_Ma.dds",ParamFilterSection::Material},
					{rootPath_ + "inspectorUI_Pa.dds",ParamFilterSection::ParameterData},
					{rootPath_ + "inspectorUI_Co.dds",ParamFilterSection::Collider},
					{rootPath_ + "inspectorUI_Emit.dds",ParamFilterSection::ParticleEmit},
					{rootPath_ + "inspectorUI_Module.dds",ParamFilterSection::ParticleModule},
				};
			for(auto& tab : allTabs_) { tab.iconTex = (void*)tm.LoadTexture(tab.iconPath).ptr; }
			tabs_ = allTabs_; // 初回は全表示
		}

		bool open = true;
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding,ImVec2(0.0f,0.0f)); // サイドバーは全高使用
		if(ImGui::Begin(panelName_.c_str(),&open)) {
			if(selectedEditor_) {
				ImGui::Text("Editor: %s",selectedEditor_->GetEditorName().c_str());
				ImGui::Separator();
				selectedEditor_->ShowImGuiInterface();
			} else {
				auto sp = selectedObject_.lock();
				if(sp && sceneObjectEditor_) {

					// レイアウト: サイドバー | コンテンツ
					if(ImGui::BeginTable("InspectorMainLayout",2,ImGuiTableFlags_Resizable | ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_SizingFixedFit)) {
						ImGui::TableSetupColumn("Sidebar",ImGuiTableColumnFlags_WidthFixed,40.0f);
						ImGui::TableSetupColumn("Content",ImGuiTableColumnFlags_None);

						ImGui::TableNextColumn();
						RenderSidebar();

						ImGui::TableNextColumn();

						// コンテンツのスクロール領域開始
						if(ImGui::BeginChild("##ContentScroll",ImVec2(0,0),false,ImGuiWindowFlags_None)) {
							ImGui::Dummy(ImVec2(0,4));
							ImGui::Indent(4.0f);

							ImGui::TextDisabled("Type: %s",sp->GetObjectTypeName().c_str());
							ImGui::SameLine();
							ImGui::Text("%s",sp->GetName().c_str());
							ImGui::Separator();
							ImGui::Spacing();

							sceneObjectEditor_->SetSceneObject(sp.get());

							// サイドバーで再構築済みの可視タブからフィルタを適用
							if(!tabs_.empty()) {
								const auto& activeTab = tabs_[std::min<std::size_t>(currentTabIndex_,tabs_.size() - 1)];
								GuiCmd::SetSectionFilter(activeTab.filterSection);
							}

							sceneObjectEditor_->ShowImGuiInterface();

							ImGui::Unindent(4.0f);
						}
						ImGui::EndChild();

						ImGui::EndTable();
					}
				} else { ImGui::Text("Nothing is selected."); }
			}
		}
		ImGui::End();
		ImGui::PopStyleVar();

		if(!open) SetShow(false);
	}

	void InspectorPanel::RenderSidebar() {
		ImGui::PushStyleColor(ImGuiCol_Button,ImVec4(0,0,0,0)); // 背景透明

		std::vector<InspectorTab> filter;
		filter.reserve(8);

		// マスター(allTabs_)から取り出す
		auto add = [&](ParamFilterSection s) {
			const auto idx = static_cast<size_t>(s);
			if(idx < allTabs_.size()) filter.push_back(allTabs_[idx]);
		};

		// デフォルトは All
		add(ParamFilterSection::All);

		auto object = selectedObject_.lock();
		if(object) {
			switch(object->GetObjectType()) {
			case ObjectType::GameObject:
				add(ParamFilterSection::Object);
				add(ParamFilterSection::Material);
				add(ParamFilterSection::ParameterData);
				add(ParamFilterSection::Collider);
				break;

			case ObjectType::Effect:
				// allTabs_ に存在しない場合は自動的にスキップされる
				add(ParamFilterSection::ParticleEmit);
				add(ParamFilterSection::Material);
				add(ParamFilterSection::ParameterData);
				add(ParamFilterSection::ParticleModule);
				break;

			case ObjectType::Camera:
				add(ParamFilterSection::ParameterData);
				break;

			case ObjectType::Light:
			case ObjectType::Event:
			default:
				break;
			}
		}

		// ここで一度だけ反映（可視タブ）
		tabs_ = std::move(filter);

		// インデックスをクランプ（サイズ0なら0のまま）
		if(tabs_.empty()) { currentTabIndex_ = 0; } else if(currentTabIndex_ >= tabs_.size()) {
			currentTabIndex_ = 0; // 失効したら All に戻す
		}

		for(int i = 0; i < (int)tabs_.size(); ++i) {
			const auto& tab        = tabs_[i];
			bool        isSelected = (currentTabIndex_ == i);

			if(isSelected) {
				// 選択中は強調
				ImGui::PushStyleColor(ImGuiCol_Button,ImVec4(0.2f,0.2f,0.2f,1.0f));
			}
			ImGui::PushID(i);

			if(ImGui::ImageButton(tab.iconTex,ImVec2(20,20))) { currentTabIndex_ = i; }
			std::string_view enumcon = CalyxUtil::EnumConverter<ParamFilterSection>::ToString(tab.filterSection);
			if(ImGui::IsItemHovered()) { ImGui::SetTooltip("%s",enumcon.data()); }

			if(isSelected) { ImGui::PopStyleColor(); }
			ImGui::PopID();

			ImGui::Spacing();
		}

		ImGui::PopStyleColor();
	}

	void InspectorPanel::RenderContent() {}

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
				// オブジェクトが有効ならセット
				sceneObjectEditor_->SetSceneObject(sp.get());
			} else {
				// 無効ならクリア
				sceneObjectEditor_->SetSceneObject(nullptr);
			}
		}
	}
} // namespace CalyxEditor