#include "InspectorPanel.h"

// engine
#include <Engine/Editor/SceneObjectEditor.h>
#include <Engine/Editor/BaseEditor.h>
#include <Engine/Assets/Texture/TextureManager.h>
#include <Engine/Foundation/Utility/Converter/EnumConverter.h>
#include <Engine/Foundation/Debug/CxAssert.h>
#include <Engine/Application/UI/Panels/AssetPanel.h>
#include <Engine/Assets/Database/AssetDatabase.h>
#include <Engine/Assets/Model/BaseModel.h>
#include <Engine/Objects/3D/Actor/BaseGameObject.h>
#include <Engine/System/Command/EditorCommand/ValueEditCommand.h>
#include <Engine/System/Command/Manager/CommandManager.h>

// externals
#include "Engine/Assets/Manager/AssetManager.h"
#include "Engine/System/Command/EditorCommand/GuiCommand/ImGuiHelper/GuiCmd.h"
#include <externals/imgui/imgui.h>

#include <algorithm>

namespace CalyxEngine {
	namespace {
		std::string LabelFromGuid(const Guid& guid, AssetType type) {
			if(!guid.isValid()) return "(none)";
			if(auto* db = AssetDatabase::GetInstance()) {
				if(const AssetRecord* record = db->Get(guid); record && record->type == type) {
					return record->sourcePath.filename().string();
				}
			}
			return guid.ToString();
		}
	}

	/////////////////////////////////////////////////////////////////////////////////////////
	//		コンストラクタ
	/////////////////////////////////////////////////////////////////////////////////////////
	InspectorPanel::InspectorPanel()
		: IEngineUI("Inspector") {}

	// ==========================================================================t==
	//		imgui描画
	// ============================================================================
	void InspectorPanel::Render() {
		if(!IsShow()) return;

		// タブの初期化（マスターを保持）
		if(allTabs_.empty()) {
			auto& tm = *AssetManager::GetInstance()->GetTextureManager();
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
			} else if(selectedObjects_.size() > 1) {
				RenderMultiSelection();
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
							const std::string displayName = sp->GetDisplayName();
							ImGui::Text("%s",displayName.c_str());
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
			case ObjectType::Object2D:
				add(ParamFilterSection::Object);
				add(ParamFilterSection::Material);
				add(ParamFilterSection::ParameterData);
				if(object->GetObjectType() == ObjectType::GameObject) {
					add(ParamFilterSection::Collider);
				}
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

			case ObjectType::Event:
				add(ParamFilterSection::Object);
				add(ParamFilterSection::Collider);
				add(ParamFilterSection::ParameterData);
				break;

			case ObjectType::Light:
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
			std::string_view enumcon = CalyxEngine::EnumConverter<ParamFilterSection>::ToString(tab.filterSection);
			if(ImGui::IsItemHovered()) { ImGui::SetTooltip("%s",enumcon.data()); }

			if(isSelected) { ImGui::PopStyleColor(); }
			ImGui::PopID();

			ImGui::Spacing();
		}

		ImGui::PopStyleColor();
	}

	void InspectorPanel::RenderContent() {}

	bool InspectorPanel::IsMultiSelectionAllBaseGameObjects() const {
		if(selectedObjects_.size() <= 1) return false;
		for(const auto& weak : selectedObjects_) {
			auto object = weak.lock();
			if(!object || !std::dynamic_pointer_cast<BaseGameObject>(object)) {
				return false;
			}
		}
		return true;
	}

	void InspectorPanel::RenderMultiSelection() {
		std::vector<std::weak_ptr<BaseGameObject>> targets;
		targets.reserve(selectedObjects_.size());

		for(const auto& weak : selectedObjects_) {
			if(auto object = weak.lock()) {
				if(auto gameObject = std::dynamic_pointer_cast<BaseGameObject>(object)) {
					targets.push_back(gameObject);
				}
			}
		}

		ImGui::Dummy(ImVec2(0, 4));
		ImGui::Indent(8.0f);
		ImGui::Text("%zu objects selected", selectedObjects_.size());
		ImGui::Separator();

		if(!IsMultiSelectionAllBaseGameObjects()) {
			ImGui::TextDisabled("Bulk editing is available only when all selected objects are BaseGameObject.");
			ImGui::Unindent(8.0f);
			return;
		}

		GuiCmd::SetSectionFilter(ParamFilterSection::Material);
		if(!GuiCmd::BeginSection(ParamFilterSection::Material)) {
			ImGui::Unindent(8.0f);
			return;
		}

		auto collectMaterialGuids = [&]() {
			std::vector<Guid> result;
			result.reserve(targets.size());
			for(const auto& weak : targets) {
				auto object = weak.lock();
				auto* model = object ? object->GetModel() : nullptr;
				result.push_back(model ? model->GetMaterialGuid() : Guid::Empty());
			}
			return result;
		};

		auto collectTextureGuids = [&]() {
			std::vector<Guid> result;
			result.reserve(targets.size());
			for(const auto& weak : targets) {
				auto object = weak.lock();
				auto* model = object ? object->GetModel() : nullptr;
				result.push_back(model ? model->GetTextureGuid() : Guid::Empty());
			}
			return result;
		};

		auto applyMaterialGuids = [targets](const std::vector<Guid>& guids) {
			for(size_t i = 0; i < targets.size() && i < guids.size(); ++i) {
				auto object = targets[i].lock();
				auto* model = object ? object->GetModel() : nullptr;
				if(model) model->SetMaterialGuid(guids[i]);
			}
		};

		auto applyTextureGuids = [targets](const std::vector<Guid>& guids) {
			for(size_t i = 0; i < targets.size() && i < guids.size(); ++i) {
				auto object = targets[i].lock();
				auto* model = object ? object->GetModel() : nullptr;
				if(model) model->SetTextureGuid(guids[i]);
			}
		};

		auto countObjectsWithModel = [&]() {
			size_t count = 0;
			for(const auto& weak : targets) {
				auto object = weak.lock();
				if(object && object->GetModel()) {
					++count;
				}
			}
			return count;
		};

		if(ImGui::TreeNodeEx("Material Asset (Bulk)", ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_DefaultOpen)) {
			Guid droppedGuid = Guid::Empty();
			if(AssetPanel::DrawAssetDropTarget(AssetType::Material, &droppedGuid)) {
				const size_t modelCount = countObjectsWithModel();
				if(modelCount == 0) {
					CX_WARN("Material cannot be applied because selected objects have no model.");
					ImGui::TreePop();
					GuiCmd::EndSection();
					ImGui::Unindent(8.0f);
					return;
				}
				if(modelCount < targets.size()) {
					CX_WARN("Material was applied only to selected objects that have a model.");
				}

				auto before = collectMaterialGuids();
				auto after = before;
				for(auto& guid : after) {
					guid = droppedGuid;
				}
				if(before != after) {
					CommandManager::GetInstance()->Execute(
						std::make_unique<ValueEditCommand<std::vector<Guid>>>("Apply Material Asset To Selection", before, after, applyMaterialGuids));
				}
			}

			const auto current = collectMaterialGuids();
			const bool same = !current.empty() && std::all_of(current.begin(), current.end(), [&](const Guid& guid) { return guid == current.front(); });
			ImGui::TextDisabled("Current: %s", same ? LabelFromGuid(current.front(), AssetType::Material).c_str() : "(mixed)");
			ImGui::TreePop();
		}

		if(ImGui::TreeNodeEx("Texture Asset (Bulk)", ImGuiTreeNodeFlags_SpanAvailWidth)) {
			Guid droppedGuid = Guid::Empty();
			if(AssetPanel::DrawAssetDropTarget(AssetType::Texture, &droppedGuid)) {
				const size_t modelCount = countObjectsWithModel();
				if(modelCount == 0) {
					CX_WARN("Texture cannot be applied because selected objects have no model.");
					ImGui::TreePop();
					GuiCmd::EndSection();
					ImGui::Unindent(8.0f);
					return;
				}
				if(modelCount < targets.size()) {
					CX_WARN("Texture was applied only to selected objects that have a model.");
				}

				auto before = collectTextureGuids();
				auto after = before;
				for(auto& guid : after) {
					guid = droppedGuid;
				}
				if(before != after) {
					CommandManager::GetInstance()->Execute(
						std::make_unique<ValueEditCommand<std::vector<Guid>>>("Apply Texture Asset To Selection", before, after, applyTextureGuids));
				}
			}

			const auto current = collectTextureGuids();
			const bool same = !current.empty() && std::all_of(current.begin(), current.end(), [&](const Guid& guid) { return guid == current.front(); });
			ImGui::TextDisabled("Current: %s", same ? LabelFromGuid(current.front(), AssetType::Texture).c_str() : "(mixed)");
			ImGui::TreePop();
		}

		GuiCmd::EndSection();
		ImGui::Unindent(8.0f);
	}

	/////////////////////////////////////////////////////////////////////////////////////////
	//		エディタセット
	/////////////////////////////////////////////////////////////////////////////////////////
	void InspectorPanel::SetSelectedEditor(BaseEditor* editor) {
		selectedEditor_ = editor;
		selectedObject_.reset();
		selectedObjects_.clear();
	}

	/////////////////////////////////////////////////////////////////////////////////////////
	//		オブジェクトセット
	/////////////////////////////////////////////////////////////////////////////////////////
	void InspectorPanel::SetSelectedObject(std::weak_ptr<SceneObject> obj) {
		selectedObject_ = obj;
		selectedObjects_.clear();
		if(auto sp = obj.lock()) {
			selectedObjects_.push_back(sp);
		}
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

	void InspectorPanel::SetSelectedObjects(const std::vector<std::weak_ptr<SceneObject>>& objects) {
		selectedObjects_.clear();
		for(const auto& weak : objects) {
			if(auto object = weak.lock()) {
				selectedObjects_.push_back(object);
			}
		}
		selectedObject_ = selectedObjects_.empty() ? std::weak_ptr<SceneObject>{} : selectedObjects_.back();
		selectedEditor_ = nullptr;

		if(sceneObjectEditor_) {
			if(auto sp = selectedObject_.lock()) {
				sceneObjectEditor_->SetSceneObject(sp.get());
			} else {
				sceneObjectEditor_->SetSceneObject(nullptr);
			}
		}
	}
} // namespace CalyxEngine
