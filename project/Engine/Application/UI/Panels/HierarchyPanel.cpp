#include "HierarchyPanel.h"
/* ========================================================================
/*  include space
/* ===================================================================== */

#include <Data/Engine/Prefab/Serializer/PrefabSerializer.h>
#include <Engine/Application/Effects/FxObject.h>
#include <Engine/Application/UI/Panels/InspectorPanel.h>
#include <Engine/Assets/Database/AssetDatabase.h>
#include <Engine/Assets/System/AssetDragPayload.h>
#include <Engine/Assets/System/AssetType.h>
#include <Engine/Assets/Texture/TextureManager.h>
#include <Engine/Objects/3D/Actor/Library/SceneObjectLibrary.h>
#include <Engine/Objects/3D/Actor/SceneObject.h>
#include <Engine/Scene/Context/SceneContext.h>

// creation headers
#include <Engine/Application/Effects/Particle/Object/ParticleSystemObject.h>
#include <Engine/Graphics/Camera/3d/Camera3d.h>
#include <Engine/Objects/3D/Actor/BaseGameObject.h>
#include <Engine/Objects/2D/Object2d/SpriteSceneObject2d.h>
#include <Engine/Objects/LightObject/DirectionalLight.h>
#include <Engine/Objects/LightObject/PointLight.h>

// lib
#include "Engine/Assets/Manager/AssetManager.h"

#include <externals/imgui/ImGuiFileDialog.h>

#include <unordered_set>
#include <string>
#include <vector>

namespace CalyxEngine {

	/* ========================================================================
	 *  include space
	 * ===================================================================== */
	namespace {

		inline const AssetDragPayload* ReadAssetPayload(const ImGuiPayload* payload) {
			if(!payload || !payload->Data || payload->DataSize != (int)sizeof(AssetDragPayload)) {
				return nullptr;
			}
			return reinterpret_cast<const AssetDragPayload*>(payload->Data);
		}

		inline const AssetRecord* GetDraggedPrefabRecord(const ImGuiPayload* payload) {
			const AssetDragPayload* assetPayload = ReadAssetPayload(payload);
			if(!assetPayload || assetPayload->type != AssetType::Prefab) {
				return nullptr;
			}

			const AssetRecord* record = AssetDatabase::GetInstance()->Get(assetPayload->guid);
			if(!record || record->type != AssetType::Prefab) {
				return nullptr;
			}
			return record;
		}

		void SetPrefabLinkRecursive(SceneObject* object, const Guid& prefabGuid) {
			if(!object || !prefabGuid.isValid()) return;
			object->SetPrefabLink(prefabGuid, object->GetGuid());
			for(const auto& child : object->GetChildren()) {
				SetPrefabLinkRecursive(child.get(), prefabGuid);
			}
		}

	} // namespace

	/* ========================================================================
	/*  ctor
	/* ===================================================================== */
	HierarchyPanel::HierarchyPanel()
		: IEngineUI("Hierarchy") {

		auto& tm = *AssetManager::GetInstance()->GetTextureManager();

		iconEye_.tex	 = (ImTextureID)tm.LoadTexture("UI/Tool/Hierarchy/eyeIcon.dds").ptr;
		iconEyeOff_.tex	 = (ImTextureID)tm.LoadTexture("UI/Tool/Hierarchy/closedEyeIcon.dds").ptr;
		iconCamera_.tex	 = (ImTextureID)tm.LoadTexture("UI/Tool/Hierarchy/camIcon.dds").ptr;
		iconGameObj_.tex = (ImTextureID)tm.LoadTexture("UI/Tool/Hierarchy/meshIcon.dds").ptr;
		iconFx_.tex		 = (ImTextureID)tm.LoadTexture("UI/Tool/Hierarchy/particleIcon.dds").ptr;

		// 追加/削除イベントにフックしてキャッシュ更新を促す
		if(auto* ctx = SceneContext::Current()) {
			ctx->AddOnObjectAddedListener([this](SceneObject*) {
				RefreshCache();
			});
			ctx->AddOnObjectRemovedListener([this](SceneObject* removed) {
				RefreshCache();
				// 選択が削除対象ならクリア
				if(IsSelected(removed)) {
					selected_.reset();
					selectedObjects_.clear();
					if(actions_) actions_->SelectObject(nullptr, false);
				}
			});
		}
	}

	/* ========================================================================
	/*  render
	/* ===================================================================== */
	void HierarchyPanel::Render() {
		// ProcessShortcuts(ImGui::GetIO());

		if(!ImGui::Begin(panelName_.c_str(), nullptr, ImGuiWindowFlags_NoDecoration)) {
			ImGui::End();
			return;
		}

		// ---------------------------------------------------------------------
		// トップバー (検索)
		// ---------------------------------------------------------------------
		{
			ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(4, 4));
			searchFilter_.Draw("##HierarchyFilter", ImGui::GetContentRegionAvail().x);
			ImGui::PopStyleVar();
		}

		lib_ = SceneContext::Current()->GetObjectLibrary();

		if(!lib_) {
			ImGui::TextUnformatted("SceneObjectLibrary not set.");
			ImGui::End();
			return;
		}

		// ===== 行ストライプ用カウンタ初期化 =====
		rowIndex_ = 0;
		// --- 消去された selected_ を無効化 ---
		{
			auto sp = selected_.lock();
			if(sp && !lib_->Contains(sp)) {
				selected_.reset();
			}
			selectedObjects_.erase(
				std::remove_if(selectedObjects_.begin(), selectedObjects_.end(),
							   [this](const std::weak_ptr<SceneObject>& weak) {
								   auto selected = weak.lock();
								   return !selected || !lib_->Contains(selected);
							   }),
				selectedObjects_.end());
		}

		// ---------------------------------------------------------------------
		// テーブル (ヘッダー + コンテンツ)
		// ---------------------------------------------------------------------
		static ImGuiTableFlags flags =
			ImGuiTableFlags_RowBg |
			ImGuiTableFlags_Resizable |
			ImGuiTableFlags_Reorderable |
			ImGuiTableFlags_ScrollY;

		if(ImGui::BeginTable("HierarchyTable", 3, flags)) {
			// カラム設定
			ImGui::TableSetupColumn("Item Label", ImGuiTableColumnFlags_NoHide);
			ImGui::TableSetupColumn("##Visible", ImGuiTableColumnFlags_WidthFixed, 24.0f);
			ImGui::TableSetupColumn("Type", ImGuiTableColumnFlags_WidthFixed, 120.0f);

			ImGui::TableHeadersRow();

			for(auto& sp : treeCache_.GetRoots(*lib_)) {
				ShowObjectRecursive(sp.get());
			}

			// 空白クリックで選択解除 (テーブル内の空白エリア)
			if(ImGui::IsWindowHovered() && ImGui::IsMouseClicked(0) && !ImGui::IsAnyItemHovered()) {
				
				selected_.reset();
				selectedObjects_.clear();
				if(actions_) actions_->SelectObject(nullptr, false);
			}

			// 右クリック空白メニュー (テーブル内の空白エリア)
			if(ImGui::IsWindowHovered() && !ImGui::IsAnyItemHovered() && ImGui::IsMouseReleased(ImGuiMouseButton_Right)) {
				ImGui::OpenPopup("BlankContextMenu");
			}

			if(ImGui::BeginDragDropTarget()) {
				if(const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("CALYX_ASSET")) {
					if(const AssetRecord* record = GetDraggedPrefabRecord(payload)) {
						auto objects = PrefabSerializer::Load(
							record->sourcePath.string(),
							PrefabSerializer::LoadOptions{false, record->guid});
						for(auto& sp : objects) {
							if(actions_) actions_->CreateObject(sp);
						}
						RefreshCache();
					}
				}
				ImGui::EndDragDropTarget();
			}

			if(ImGui::BeginPopup("BlankContextMenu")) {
				if(ImGui::BeginMenu("Create")) {
					auto createRoot = [&](std::shared_ptr<SceneObject> obj) {
						if(actions_) actions_->CreateObject(obj);
					};

					if(ImGui::MenuItem("Empty Scene Object")) createRoot(std::make_shared<SceneObject>());
					if(ImGui::MenuItem("Camera")) createRoot(std::make_shared<Camera3d>());
					if(ImGui::BeginMenu("Light")) {
						if(ImGui::MenuItem("Directional Light")) createRoot(std::make_shared<DirectionalLight>());
						if(ImGui::MenuItem("Point Light")) createRoot(std::make_shared<PointLight>());
						ImGui::EndMenu();
					}
					if(ImGui::MenuItem("Mesh Object")) createRoot(std::make_shared<BaseGameObject>());
					if(ImGui::BeginMenu("2D")) {
						if(ImGui::MenuItem("Sprite 2D")) createRoot(std::make_shared<CalyxEngine::SpriteSceneObject2d>());
						if(ImGui::MenuItem("Animated Sprite 2D")) createRoot(std::make_shared<CalyxEngine::AnimatedSpriteSceneObject2d>());
						ImGui::EndMenu();
					}
					if(ImGui::BeginMenu("Effect")) {
						if(ImGui::MenuItem("Fx Object")) createRoot(std::make_shared<CalyxEngine::FxObject>());
						if(ImGui::MenuItem("Particle System")) createRoot(std::make_shared<CalyxEngine::ParticleSystemObject>());
						ImGui::EndMenu();
					}
					ImGui::EndMenu();
				}
				ImGui::Separator();
				if(ImGui::MenuItem("Load Prefab")) showLoadPrefabDlg_ = true;
				ImGui::EndPopup();
			}

			ImGui::EndTable();
		}

		// --- Prefab Dialog ---
		if(showLoadPrefabDlg_) {
			IGFD::FileDialogConfig cfg;
			cfg.path = "Resources/Assets/Prefabs/";
			ImGuiFileDialog::Instance()->OpenDialog("LoadPrefabDlg", "Load Prefab", ".prefab", cfg);
			showLoadPrefabDlg_ = false;
		}

		if(showSavePrefabDlg_) {
			IGFD::FileDialogConfig cfg;
			cfg.path = "Resources/Assets/Prefabs/";
			ImGuiFileDialog::Instance()->OpenDialog("SavePrefabDlg", "Save Prefab", ".prefab", cfg);
			showSavePrefabDlg_ = false;
		}

		// Save
		if(ImGuiFileDialog::Instance()->Display("SavePrefabDlg")) {
			if(ImGuiFileDialog::Instance()->IsOk() && prefabSaveTarget_) {
				const std::string path = ImGuiFileDialog::Instance()->GetFilePathName();
				if(PrefabSerializer::Save(
					   {prefabSaveTarget_},
					   path,
					   PrefabSerializer::SaveOptions{true})) {
					const Guid prefabGuid = AssetDatabase::GetInstance()->RegisterOrUpdate(path, AssetType::Prefab);
					SetPrefabLinkRecursive(prefabSaveTarget_, prefabGuid);
					AssetDatabase::GetInstance()->Scan();
				}
			}
			ImGuiFileDialog::Instance()->Close();
			prefabSaveTarget_ = nullptr;
		}

		// Load
		if(ImGuiFileDialog::Instance()->Display("LoadPrefabDlg")) {
			if(ImGuiFileDialog::Instance()->IsOk()) {
				const std::string path = ImGuiFileDialog::Instance()->GetFilePathName();
				const Guid prefabGuid = AssetDatabase::GetInstance()->RegisterOrUpdate(path, AssetType::Prefab);
				auto vec = PrefabSerializer::Load(
					path,
					PrefabSerializer::LoadOptions{false, prefabGuid});

				for(auto& sp : vec) {
					if(lib_ && actions_) {
						actions_->CreateObject(sp);
					}
				}
			}
			ImGuiFileDialog::Instance()->Close();
		}

		ImGui::End();
	}

	/* ========================================================================
	/*  recursive UI
	/* ===================================================================== */
	void HierarchyPanel::ShowObjectRecursive(SceneObject* obj) {
		if(!obj || obj->IsTransient()) return;

		if(searchFilter_.IsActive()) {
			if(!PassFilterRecursive(obj)) return;
			ImGui::SetNextItemOpen(true, ImGuiCond_Always);
		}

		ImGui::PushID(obj);

		const bool open = DrawNode(obj);

		if(open) {
			auto	   renameSP		  = renameTarget_.lock();
			const bool isRenamingThis = (renaming_ && renameSP.get() == obj);

			if(!isRenamingThis) {
				for(auto& child : treeCache_.GetChildren(*obj)) {
					ShowObjectRecursive(child.get());
				}
			}

			ImGui::TreePop();
		}

		ImGui::PopID();
	}

	bool HierarchyPanel::DrawNode(SceneObject* obj) {

		ImGui::TableNextRow();

		// ---------------------------------------------------------------------
		// カラム 0: アイテム名 (ノード)
		// ---------------------------------------------------------------------
		ImGui::TableSetColumnIndex(0);

		ImTextureID typeTex = GetTypeIcon(obj->GetObjectType());
		// リネームロジック
		auto renameSP		= renameTarget_.lock();
		bool isRenamingThis = (renaming_ && renameSP.get() == obj);

		bool isSelected	 = IsSelected(obj);

		// 16px アイコンを使用
		float iconSize = 16.0f;

		ImGuiTreeNodeFlags flags =
			ImGuiTreeNodeFlags_OpenOnArrow |
			ImGuiTreeNodeFlags_SpanAvailWidth |
			ImGuiTreeNodeFlags_SpanFullWidth |
			(isSelected ? ImGuiTreeNodeFlags_Selected : 0) |
			(obj->GetChildren().empty() ? ImGuiTreeNodeFlags_Leaf : 0);

		bool open = false;

		if(isRenamingThis) {
			// Rename Mode
			open = ImGui::TreeNodeEx("##rename_dummy", flags | ImGuiTreeNodeFlags_AllowItemOverlap, "");
			ImGui::SameLine();

			ImGui::SetKeyboardFocusHere();
			char buf[256];
			snprintf(buf, sizeof(buf), "%s", renameBuf_.c_str());

			ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));
			if(ImGui::InputText("##rename", buf, sizeof(buf), ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_AutoSelectAll)) {
				renameBuf_ = buf;
				CommitRename();
			}
			ImGui::PopStyleVar();

			if(ImGui::IsItemDeactivatedAfterEdit()) {
				renameBuf_ = buf;
				CommitRename();
			}
			if(ImGui::IsKeyPressed(ImGuiKey_Escape)) CancelRename();

		} else {
			// 通常モード
			open = ImGui::TreeNodeEx(obj, flags, "");

			// インタラクション
			if(ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen()) {
				HandleNodeSelectionClick(obj);
			}
			if(ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left) && actions_) {
				actions_->FocusObject(obj->shared_from_this());
			}

			// ドラッグ＆ドロップ
			if(ImGui::BeginDragDropSource()) {
				SceneObject* drag = obj;
				ImGui::SetDragDropPayload("SceneObjectPtr", &drag, sizeof(SceneObject*));
				const std::string displayName = obj->GetDisplayName();
				ImGui::Text("%s", displayName.c_str());
				ImGui::EndDragDropSource();
			}

			if(ImGui::BeginDragDropTarget()) {
				if(const ImGuiPayload* pl = ImGui::AcceptDragDropPayload("SceneObjectPtr")) {
					SceneObject* drag = *reinterpret_cast<SceneObject**>(pl->Data);
					if(drag && drag != obj) {
						auto dragSP = drag->shared_from_this();
						auto objSP	= obj->shared_from_this();
						if(lib_->Contains(dragSP) && lib_->Contains(objSP) && !IsDescendantOf(obj, drag)) {
							drag->SetParent(objSP);
							RefreshCache();
						}
					}
				}
				if(const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("CALYX_ASSET")) {
					if(const AssetRecord* record = GetDraggedPrefabRecord(payload)) {
						auto objects = PrefabSerializer::Load(
							record->sourcePath.string(),
							PrefabSerializer::LoadOptions{false, record->guid});

						std::unordered_set<SceneObject*> loaded;
						loaded.reserve(objects.size());
						for(auto& sp : objects) {
							if(sp) loaded.insert(sp.get());
						}

						auto parent = obj->shared_from_this();
						for(auto& sp : objects) {
							if(!sp) continue;
							auto existingParent = sp->GetParent();
							if(!existingParent || !loaded.contains(existingParent.get())) {
								sp->SetParent(parent);
							}
						}

						for(auto& sp : objects) {
							if(actions_) actions_->CreateObject(sp);
						}
						RefreshCache();
					}
				}
				ImGui::EndDragDropTarget();
			}

			// コンテキストメニュー
			if(ImGui::BeginPopupContextItem("SOContext")) { // アイテム上で右クリック
				if(ImGui::BeginMenu("Create Child")) {
					auto createChild = [&](std::shared_ptr<SceneObject> child) {
						child->SetParent(obj->shared_from_this());
						if(actions_) actions_->CreateObject(child);
					};

					if(ImGui::MenuItem("Empty Scene Object")) createChild(std::make_shared<SceneObject>());
					if(ImGui::MenuItem("Camera")) createChild(std::make_shared<Camera3d>());
					if(ImGui::BeginMenu("Light")) {
						if(ImGui::MenuItem("Directional Light")) createChild(std::make_shared<DirectionalLight>());
						if(ImGui::MenuItem("Point Light")) createChild(std::make_shared<PointLight>());
						ImGui::EndMenu();
					}
					if(ImGui::MenuItem("Mesh Object")) createChild(std::make_shared<BaseGameObject>());
					if(ImGui::BeginMenu("2D")) {
						if(ImGui::MenuItem("Sprite 2D")) createChild(std::make_shared<CalyxEngine::SpriteSceneObject2d>());
						if(ImGui::MenuItem("Animated Sprite 2D")) createChild(std::make_shared<CalyxEngine::AnimatedSpriteSceneObject2d>());
						ImGui::EndMenu();
					}
					if(ImGui::BeginMenu("Effect")) {
						if(ImGui::MenuItem("Fx Object")) createChild(std::make_shared<CalyxEngine::FxObject>());
						if(ImGui::MenuItem("Particle System")) createChild(std::make_shared<CalyxEngine::ParticleSystemObject>());
						ImGui::EndMenu();
					}

					ImGui::EndMenu();
				}
				ImGui::Separator();
				if(ImGui::MenuItem("Rename")) BeginRename(obj);
				if(ImGui::MenuItem("Delete") && actions_) {
					if(auto sp = obj->shared_from_this()) actions_->DeleteObject(sp);
				}
				ImGui::Separator();
				if(ImGui::MenuItem("Create Prefab")) {
					prefabSaveTarget_  = obj;
					showSavePrefabDlg_ = true;
				}
				if(obj->IsPrefabInstanceObject() && actions_) {
					if(ImGui::MenuItem("Apply Prefab Overrides")) {
						if(auto sp = obj->shared_from_this()) actions_->ApplyPrefabOverrides(sp);
					}
				}
				ImGui::EndPopup();
			}

			// ショートカット (F2)
			if(isSelected && ImGui::IsWindowFocused() && ImGui::IsKeyPressed(ImGuiKey_F2)) {
				BeginRename(obj);
			}

			// ノード上にアイコンとテキストを描画
			ImGui::SameLine();
			const bool prefabInstance = obj->IsPrefabInstanceObject();
			if(prefabInstance) {
				ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.35f, 0.62f, 1.0f, 1.0f));
			}
			if(typeTex) {
				ImGui::Image(typeTex, ImVec2(iconSize, iconSize));
				ImGui::SameLine();
			}
			const std::string displayName = obj->GetDisplayName();
			ImGui::TextUnformatted(displayName.c_str());
			if(prefabInstance) {
				ImGui::PopStyleColor();
			}
		}

		// ---------------------------------------------------------------------
		// カラム 1: 表示切り替え (目)
		// ---------------------------------------------------------------------
		ImGui::TableSetColumnIndex(1);

		auto& eyeIcon = obj->IsDrawEnable() ? iconEye_ : iconEyeOff_;

		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));
		if(ImGui::ImageButton(eyeIcon.tex, ImVec2(iconSize, iconSize))) {
			obj->SetDrawEnable(!obj->IsDrawEnable());
		}
		ImGui::PopStyleVar();
		ImGui::PopStyleColor();

		// ---------------------------------------------------------------------
		// カラム 2: タイプ情報
		// ---------------------------------------------------------------------
		ImGui::TableSetColumnIndex(2);
		if(obj->IsPrefabInstanceObject()) {
			ImGui::TextColored(ImVec4(0.35f, 0.62f, 1.0f, 1.0f), "%s", GetTypeLabel(obj->GetObjectType()));
		} else {
			ImGui::TextDisabled("%s", GetTypeLabel(obj->GetObjectType()));
		}

		return open;
	}

	/* ========================================================================
	/*  utils
	/* ===================================================================== */
	void HierarchyPanel::HandleNodeSelectionClick(SceneObject* obj) {
		if(!obj) return;

		try {
			auto sp = obj->shared_from_this();
			const bool toggle = ImGui::GetIO().KeyCtrl;
			if(toggle && IsSelected(obj)) {
				selectedObjects_.erase(
					std::remove_if(selectedObjects_.begin(), selectedObjects_.end(),
								   [obj](const std::weak_ptr<SceneObject>& weak) {
									   return weak.lock().get() == obj;
								   }),
					selectedObjects_.end());
				selected_ = selectedObjects_.empty() ? std::weak_ptr<SceneObject>{} : selectedObjects_.back();
			} else if(toggle) {
				selectedObjects_.push_back(sp);
				selected_ = sp;
			} else {
				selectedObjects_.clear();
				selectedObjects_.push_back(sp);
				selected_ = sp;
			}
			if(actions_) actions_->SelectObject(sp, toggle);
		} catch(...) {
			selected_.reset();
			selectedObjects_.clear();
		}
	}

	ImTextureID HierarchyPanel::GetTypeIcon(ObjectType type) const {
		switch(type) {
		case ObjectType::Camera:
			return iconCamera_.tex;
		case ObjectType::Light:
			return iconLight_.tex;
		case ObjectType::GameObject:
		case ObjectType::Object2D:
		case ObjectType::Event:
			return iconGameObj_.tex;
		case ObjectType::Effect:
			return iconFx_.tex;
		default:
			return nullptr;
		}
	}

	const char* HierarchyPanel::GetTypeLabel(ObjectType type) const {
		switch(type) {
		case ObjectType::Camera:
			return "Camera";
		case ObjectType::Light:
			return "Light";
		case ObjectType::GameObject:
			return "Mesh";
		case ObjectType::Object2D:
			return "2D";
		case ObjectType::Effect:
			return "Effect";
		case ObjectType::Event:
			return "Event";
		default:
			return "Object";
		}
	}

	bool HierarchyPanel::IsDescendantOf(SceneObject* parent, SceneObject* child) {
		if(!child) return false;

		for(auto p = child->GetParent(); p; p = p->GetParent()) {
			if(p.get() == parent) return true;
		}
		return false;
	}

	bool HierarchyPanel::PassFilterRecursive(SceneObject* obj) const {
		if(!obj) return false;

		// 自分自身が通過するか？
		if(searchFilter_.PassFilter(obj->GetName().c_str())) return true;

		// 子のいずれかが通過するか？
		for(auto& child : obj->GetChildren()) {
			if(PassFilterRecursive(child.get())) return true;
		}

		return false;
	}

	const std::string& HierarchyPanel::GetPanelName() const {
		return panelName_;
	}

	void HierarchyPanel::SetSelectedObjects(const std::vector<std::shared_ptr<SceneObject>>& objects) {
		selectedObjects_.clear();
		for(const auto& object : objects) {
			if(!object) continue;
			selectedObjects_.push_back(object);
		}
		selected_ = selectedObjects_.empty() ? std::weak_ptr<SceneObject>{} : selectedObjects_.back();
	}

	bool HierarchyPanel::IsSelected(SceneObject* obj) const {
		if(!obj) return false;
		for(const auto& weak : selectedObjects_) {
			if(weak.lock().get() == obj) {
				return true;
			}
		}
		return false;
	}

	/* ========================================================================
	/*  rename
	/* ===================================================================== */
	void HierarchyPanel::BeginRename(SceneObject* obj) {

		if(!obj) return;

		renaming_	  = true;
		renameTarget_ = obj->shared_from_this();
		renameBuf_	  = obj->GetName();
	}

	void HierarchyPanel::CancelRename() {
		renaming_ = false;
		renameTarget_.reset();
		renameBuf_.clear();
	}

	void HierarchyPanel::CommitRename() {

		auto target = renameTarget_.lock();
		if(!renaming_ || !target) {
			CancelRename();
			return;
		}

		std::string newName = renameBuf_;

		auto l = newName.find_first_not_of(" \t\r\n");
		auto r = newName.find_last_not_of(" \t\r\n");

		if(l == std::string::npos)
			newName.clear();
		else
			newName = newName.substr(l, r - l + 1);

		if(newName.empty()) {
			CancelRename();
			return;
		}

		if(auto sp = renameTarget_.lock()) {
			if(actions_) {
				actions_->RenameObject(sp, newName);
			} else {
				sp->SetName(newName, sp->GetObjectType());
			}
			RefreshCache();
		}

		CancelRename();
	}

} // namespace CalyxEngine
