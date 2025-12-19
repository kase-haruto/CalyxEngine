#include "HierarchyPanel.h"
/* ========================================================================
/*  include space
/* ===================================================================== */

#include <Data/Engine/Prefab/Serializer/PrefabSerializer.h>
#include <Engine/Application/UI/Panels/InspectorPanel.h>
#include <Engine/Assets/Texture/TextureManager.h>
#include <Engine/Objects/3D/Actor/SceneObject.h>
#include <Engine/Scene/Context/SceneContext.h>
#include <Engine/objects/3D/Actor/Library/SceneObjectLibrary.h>

// lib
#include <externals/imgui/ImGuiFileDialog.h>

#include <algorithm>
#include <string>
#include <vector>

namespace CalyxEditor {

	/* ========================================================================
	/*  local helpers
	/* ===================================================================== */
	namespace {

		inline int TypePriority(ObjectType t) {
			switch(t) {
			case ObjectType::Camera:
				return 0;
			case ObjectType::Light:
				return 1;
			case ObjectType::GameObject:
				return 2;
			case ObjectType::Effect:
				return 3;
			default:
				return 9;
			}
		}

		inline bool LessByTypeThenName(const std::shared_ptr<SceneObject>& a,
									   const std::shared_ptr<SceneObject>& b) {
			int pa = TypePriority(a->GetObjectType());
			int pb = TypePriority(b->GetObjectType());
			if(pa != pb) return pa < pb;
			return a->GetName() < b->GetName();
		}

	} // namespace

	/* ========================================================================
	/*  ctor
	/* ===================================================================== */
	HierarchyPanel::HierarchyPanel()
		: IEngineUI("Hierarchy") {

		auto& tm = *TextureManager::GetInstance();

		iconEye_.tex	 = (ImTextureID)tm.LoadTexture("UI/Tool/Hierarchy/eyeIcon.png").ptr;
		iconEyeOff_.tex	 = (ImTextureID)tm.LoadTexture("UI/Tool/Hierarchy/closedEyeIcon.png").ptr;
		iconCamera_.tex	 = (ImTextureID)tm.LoadTexture("UI/Tool/Hierarchy/camIcon.png").ptr;
		iconGameObj_.tex = (ImTextureID)tm.LoadTexture("UI/Tool/Hierarchy/meshIcon.png").ptr;
		iconFx_.tex		 = (ImTextureID)tm.LoadTexture("UI/Tool/Hierarchy/particleIcon.png").ptr;
		iconLight_.tex	 = (ImTextureID)tm.LoadTexture("UI/Tool/Hierarchy/lightIcon.png").ptr;
	}

	/* ========================================================================
	/*  render
	/* ===================================================================== */
	void HierarchyPanel::Render() {

		bool open = true;
		ImGui::Begin(panelName_.c_str(), &open, ImGuiWindowFlags_NoDecoration);
		ImGui::Text("Scene Hierarchy");

		lib_ = SceneContext::Current()->GetObjectLibrary();

		if(!lib_) {
			ImGui::Text("SceneObjectLibrary not set.");
			ImGui::End();
			if(!open) SetShow(false);
			return;
		}

		// --- 消去された selected_ を無効化 ---
		{
			auto sp = selected_.lock();
			if(sp && !lib_->Contains(sp)) {
				selected_.reset();
			}
		}

		// --- root 探索 ---
		std::vector<std::shared_ptr<SceneObject>> roots;
		auto									  all = lib_->GetAllObjectsShared();
		roots.reserve(all.size());

		for(auto& sp : all) {
			if(!sp) continue;

			auto parent = sp->GetParent();
			if(!parent || !lib_->Contains(parent)) {
				roots.push_back(sp);
			}
		}

		std::sort(roots.begin(), roots.end(), LessByTypeThenName);

		// --- draw ---
		for(auto& sp : roots) {
			ShowObjectRecursive(sp.get());
		}

		// 空白クリックで選択解除
		if(ImGui::IsWindowHovered() && ImGui::IsMouseClicked(0) && !ImGui::IsAnyItemHovered()) {
			selected_.reset();
			if(onSelect_) onSelect_(nullptr);
		}

		// 右クリック空白メニュー
		if(ImGui::IsWindowHovered() && !ImGui::IsAnyItemHovered() && ImGui::IsMouseReleased(ImGuiMouseButton_Right)) {
			ImGui::OpenPopup("BlankContextMenu");
		}

		if(ImGui::BeginPopup("BlankContextMenu")) {
			if(ImGui::MenuItem("Load Prefab")) showLoadPrefabDlg_ = true;
			ImGui::EndPopup();
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
				PrefabSerializer::Save({prefabSaveTarget_},
									   ImGuiFileDialog::Instance()->GetFilePathName());
			}
			ImGuiFileDialog::Instance()->Close();
			prefabSaveTarget_ = nullptr;
		}

		// Load
		if(ImGuiFileDialog::Instance()->Display("LoadPrefabDlg")) {
			if(ImGuiFileDialog::Instance()->IsOk()) {
				auto vec = PrefabSerializer::Load(
					ImGuiFileDialog::Instance()->GetFilePathName());

				for(auto& up : vec) {
					if(lib_ && onCreate_) {
						onCreate_(std::shared_ptr<SceneObject>(std::move(up)));
					}
				}
			}
			ImGuiFileDialog::Instance()->Close();
		}

		ImGui::End();
		if(!open) SetShow(false);
	}

	/* ========================================================================
	/*  recursive UI
	/* ===================================================================== */
	void HierarchyPanel::ShowObjectRecursive(SceneObject* obj) {

		auto selectedPtr = selected_.lock();
		bool sel		 = (selectedPtr.get() == obj);

		ImGui::PushID(obj);

		// 表示トグル
		auto eye = obj->IsDrawEnable() ? iconEye_ : iconEyeOff_;
		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, {0, 0});
		if(ImGui::ImageButton(eye.tex, eye.size)) {
			obj->SetDrawEnable(!obj->IsDrawEnable());
		}
		ImGui::PopStyleVar();
		ImGui::SameLine();

		// 種類アイコン
		ImTextureID typeIcon = nullptr;
		switch(obj->GetObjectType()) {
		case ObjectType::Camera:
			typeIcon = iconCamera_.tex;
			break;
		case ObjectType::Light:
			typeIcon = iconLight_.tex;
			break;
		case ObjectType::GameObject:
			typeIcon = iconGameObj_.tex;
			break;
		case ObjectType::Effect:
			typeIcon = iconFx_.tex;
			break;
		}
		if(typeIcon) {
			ImGui::Image(typeIcon, eye.size);
			ImGui::SameLine();
		}

		// rename 中か
		auto renameSP		= renameTarget_.lock();
		bool isRenamingThis = (renaming_ && renameSP.get() == obj);

		ImGuiTreeNodeFlags fl =
			ImGuiTreeNodeFlags_OpenOnArrow |
			(obj->GetChildren().empty() ? ImGuiTreeNodeFlags_Leaf : 0) |
			(sel ? ImGuiTreeNodeFlags_Selected : 0);

		bool open = false;

		if(isRenamingThis) {

			open = ImGui::TreeNodeEx("##renameNode", fl | ImGuiTreeNodeFlags_Leaf);
			ImGui::SameLine();

			if(!ImGui::IsAnyItemActive()) {
				ImGui::SetKeyboardFocusHere();
			}

			char buf[256];
			snprintf(buf, sizeof(buf), "%s", renameBuf_.c_str());

			ImGuiInputTextFlags flags =
				ImGuiInputTextFlags_AutoSelectAll |
				ImGuiInputTextFlags_EnterReturnsTrue;

			if(ImGui::InputText("##rename", buf, sizeof(buf), flags)) {
				renameBuf_ = buf;
				CommitRename();
			} else {
				if(ImGui::IsItemFocused() && ImGui::IsKeyPressed(ImGuiKey_Escape)) {
					CancelRename();
				}
				if(ImGui::IsItemDeactivatedAfterEdit()) {
					renameBuf_ = buf;
					CommitRename();
				}
			}
		} else {

			open = ImGui::TreeNodeEx(obj->GetName().c_str(), fl);

			if(ImGui::IsItemClicked()) {
				if(auto sp = obj->shared_from_this()) {
					selected_ = sp;
					if(onSelect_) onSelect_(sp);
				}
			}

			// F2 rename
			if(sel && ImGui::IsWindowFocused() && ImGui::IsKeyPressed(ImGuiKey_F2)) {
				BeginRename(obj);
			}

			// double click rename
			if(ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
				BeginRename(obj);
			}

			// context menu
			if(ImGui::BeginPopupContextItem("SOContext")) {

				if(ImGui::MenuItem("Rename")) {
					BeginRename(obj);
				}

				if(ImGui::MenuItem("Delete") && onDelete_) {
					if(auto sp = obj->shared_from_this())
						onDelete_(sp);
				}

				if(ImGui::MenuItem("Create Prefab")) {
					prefabSaveTarget_  = obj;
					showSavePrefabDlg_ = true;
				}

				ImGui::EndPopup();
			}
		}

		// Drag & Drop
		if(ImGui::BeginDragDropSource()) {
			SceneObject* drag = obj;
			ImGui::SetDragDropPayload("SceneObjectPtr", &drag, sizeof(SceneObject*));
			ImGui::Text("%s", obj->GetName().c_str());
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
					}
				}
			}
			ImGui::EndDragDropTarget();
		}

		if(open) {
			if(!isRenamingThis) {
				std::vector<std::shared_ptr<SceneObject>> sortedChildren;
				for(auto& ch : obj->GetChildren()) {
					if(ch) sortedChildren.push_back(ch);
				}
				std::sort(sortedChildren.begin(), sortedChildren.end(), LessByTypeThenName);

				for(auto& ch : sortedChildren) {
					ShowObjectRecursive(ch.get());
				}
			}
			ImGui::TreePop();
		}

		ImGui::PopID();
	}

	/* ========================================================================
	/*  utils
	/* ===================================================================== */
	bool HierarchyPanel::IsDescendantOf(SceneObject* parent, SceneObject* child) {
		if(!child) return false;

		for(auto p = child->GetParent(); p; p = p->GetParent()) {
			if(p.get() == parent) return true;
		}
		return false;
	}

	const std::string& HierarchyPanel::GetPanelName() const {
		return panelName_;
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
			if(onRename_) {
				onRename_(sp, newName);
			} else {
				sp->SetName(newName, sp->GetObjectType());
			}
		}

		CancelRename();
	}

} // namespace CalyxEditor