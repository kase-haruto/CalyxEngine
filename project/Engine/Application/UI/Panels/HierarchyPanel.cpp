/* ========================================================================
/*  include space
/* ===================================================================== */

// engine
#include <Engine/Application/UI/Panels/HierarchyPanel.h>
#include <Engine/Application/UI/Panels/InspectorPanel.h>
#include <Engine/objects/3D/Actor/Library/SceneObjectLibrary.h>
#include <Engine/Objects/3D/Actor/SceneObject.h>
#include <Engine/Assets/Texture/TextureManager.h>
#include <Data/Engine/Prefab/Serializer/PrefabSerializer.h>
#include <Engine/Scene/Context/SceneContext.h>
// lib
#include <externals/imgui/imgui.h>
#include <externals/imgui/ImGuiFileDialog.h>

#include <algorithm>
#include <vector>
#include <string>

/* ========================================================================
/*  local helpers
/* ===================================================================== */
namespace {
	inline int TypePriority(ObjectType t) {
		switch (t) {
			case ObjectType::Camera:         return 0;
			case ObjectType::Light:          return 1;
			case ObjectType::GameObject:     return 2;
			case ObjectType::ParticleSystem: return 3;
			default:                         return 9;
		}
	}

	inline bool LessByTypeThenName(const std::shared_ptr<SceneObject>& a,
								   const std::shared_ptr<SceneObject>& b) {
		const int pa = TypePriority(a->GetObjectType());
		const int pb = TypePriority(b->GetObjectType());
		if (pa != pb) return pa < pb;
		const auto& na = a->GetName();
		const auto& nb = b->GetName();
		return na < nb;
	}
}

/* ========================================================================
/*  ctor
/* ===================================================================== */
HierarchyPanel::HierarchyPanel() : IEngineUI("Hierarchy") {
	auto& tm = *TextureManager::GetInstance();
	iconEye_.tex = (ImTextureID)tm.LoadTexture("UI/Tool/Hierarchy/eyeIcon.png").ptr;
	iconEyeOff_.tex = (ImTextureID)tm.LoadTexture("UI/Tool/Hierarchy/closedEyeIcon.png").ptr;
	iconCamera_.tex = (ImTextureID)tm.LoadTexture("UI/Tool/Hierarchy/camIcon.png").ptr;
	iconGameObj_.tex = (ImTextureID)tm.LoadTexture("UI/Tool/Hierarchy/meshIcon.png").ptr;
	iconFx_.tex = (ImTextureID)tm.LoadTexture("UI/Tool/Hierarchy/particleIcon.png").ptr;
	iconLight_.tex = (ImTextureID)tm.LoadTexture("UI/Tool/Hierarchy/lightIcon.png").ptr;
}

/* ========================================================================
/*  render
/* ===================================================================== */
void HierarchyPanel::Render() {
	bool open = true;
	ImGui::Begin(panelName_.c_str(), &open, ImGuiWindowFlags_NoDecoration);
	ImGui::Text("Scene Hierarchy");

	lib_ = SceneContext::Current()->GetObjectLibrary();

	if (!lib_) {
		ImGui::Text("SceneObjectLibrary not set.");
		ImGui::End();
		if (!open) SetShow(false);
		return;
	}

	// ルート候補収集（親が無い or 親がシーンに存在しない物）
	std::vector<std::shared_ptr<SceneObject>> roots;
	roots.reserve(lib_->GetAllObjectsShared().size());
	for (const auto& sp : lib_->GetAllObjectsShared()) {
		if (!sp) continue;
		auto parent = sp->GetParent();
		if (!parent || !lib_->Contains(parent)) {
			roots.push_back(sp);
		}
	}
	// タイプ→名前で安定ソート
	std::sort(roots.begin(), roots.end(), LessByTypeThenName);

	// 表示
	for (const auto& sp : roots) {
		ShowObjectRecursive(sp.get());
	}

	// 空白クリックで選択解除
	if (ImGui::IsWindowHovered() && ImGui::IsMouseClicked(0) && !ImGui::IsAnyItemHovered()) {
		selected_.reset();
		if (onSelect_) onSelect_(nullptr);
	}

	// 右クリック空白メニュー
	if (ImGui::IsWindowHovered() && !ImGui::IsAnyItemHovered() && ImGui::IsMouseReleased(ImGuiMouseButton_Right)) {
		ImGui::OpenPopup("BlankContextMenu");
	}
	if (ImGui::BeginPopup("BlankContextMenu")) {
		if (ImGui::MenuItem("Load Prefab")) showLoadPrefabDlg_ = true;
		ImGui::EndPopup();
	}

	// ---------- ダイアログ ----------
	if (showLoadPrefabDlg_) {
		IGFD::FileDialogConfig cfg;
		cfg.path = "Resources/Assets/Prefabs/";
		ImGuiFileDialog::Instance()->OpenDialog("LoadPrefabDlg", "Load Prefab", ".prefab", cfg);
		showLoadPrefabDlg_ = false;
	}
	if (showSavePrefabDlg_) {
		IGFD::FileDialogConfig cfg;
		cfg.path = "Resources/Assets/Prefabs/";
		ImGuiFileDialog::Instance()->OpenDialog("SavePrefabDlg", "Save Prefab", ".prefab", cfg);
		showSavePrefabDlg_ = false;
	}

	// Save
	if (ImGuiFileDialog::Instance()->Display("SavePrefabDlg")) {
		if (ImGuiFileDialog::Instance()->IsOk() && prefabSaveTarget_) {
			PrefabSerializer::Save({ prefabSaveTarget_ }, ImGuiFileDialog::Instance()->GetFilePathName());
		}
		ImGuiFileDialog::Instance()->Close();
		prefabSaveTarget_ = nullptr;
	}

	// Load
	if (ImGuiFileDialog::Instance()->Display("LoadPrefabDlg")) {
		if (ImGuiFileDialog::Instance()->IsOk()) {
			auto vec = PrefabSerializer::Load(ImGuiFileDialog::Instance()->GetFilePathName());
			for (auto& up : vec) {
				if (lib_ && onCreate_) {
					onCreate_(std::shared_ptr<SceneObject>(std::move(up)));
				}
			}
		}
		ImGuiFileDialog::Instance()->Close();
	}

	ImGui::End();
	if (!open) SetShow(false);
}

/* ========================================================================
/*  recursive draw (sorted children)
/* ===================================================================== */
void HierarchyPanel::ShowObjectRecursive(SceneObject* obj) {
	bool sel = (selected_.get() == obj);
	ImGui::PushID(obj);

	// 表示トグル
	auto eye = obj->IsDrawEnable() ? iconEye_ : iconEyeOff_;
	ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, { 0,0 });
	if (ImGui::ImageButton(eye.tex, eye.size)) {
		obj->SetDrawEnable(!obj->IsDrawEnable());
	}
	ImGui::PopStyleVar();
	ImGui::SameLine();

	// 種類アイコン
	ImTextureID typeI = nullptr;
	switch (obj->GetObjectType()) {
		case ObjectType::Camera:         typeI = iconCamera_.tex;  break;
		case ObjectType::Light:          typeI = iconLight_.tex;   break;
		case ObjectType::GameObject:     typeI = iconGameObj_.tex; break;
		case ObjectType::ParticleSystem: typeI = iconFx_.tex;      break;
		default: break;
	}
	if (typeI) {
		ImGui::Image(typeI, eye.size);
		ImGui::SameLine();
	}

	ImGuiTreeNodeFlags fl =
		ImGuiTreeNodeFlags_OpenOnArrow |
		(obj->GetChildren().empty() ? ImGuiTreeNodeFlags_Leaf : 0) |
		(sel ? ImGuiTreeNodeFlags_Selected : 0);

	bool open = ImGui::TreeNodeEx(obj->GetName().c_str(), fl);

	if (ImGui::IsItemClicked()) {
		selected_ = obj->shared_from_this();
		if (onSelect_) onSelect_(selected_);
	}

	// コンテキスト
	if (ImGui::BeginPopupContextItem("SOContext")) {
		if (ImGui::MenuItem("Delete") && onDelete_) onDelete_(selected_);
		if (ImGui::MenuItem("Create Prefab")) {
			prefabSaveTarget_ = obj;
			showSavePrefabDlg_ = true;
		}
		ImGui::EndPopup();
	}

	// Drag&Drop
	if (ImGui::BeginDragDropSource()) {
		SceneObject* drag = obj;
		ImGui::SetDragDropPayload("SceneObjectPtr", &drag, sizeof(SceneObject*));
		ImGui::Text("%s", obj->GetName().c_str());
		ImGui::EndDragDropSource();
	}
	if (ImGui::BeginDragDropTarget()) {
		if (const ImGuiPayload* pl = ImGui::AcceptDragDropPayload("SceneObjectPtr")) {
			SceneObject* drag = *reinterpret_cast<SceneObject**>(pl->Data);
			if (drag && drag != obj && !IsDescendantOf(obj, drag)) {
				drag->SetParent(obj->shared_from_this());
			}
		}
		ImGui::EndDragDropTarget();
	}

	// 再帰（子もタイプ→名前順に並べ替え）
	if (open) {
		std::vector<std::shared_ptr<SceneObject>> sortedChildren;
		sortedChildren.reserve(obj->GetChildren().size());
		for (auto& ch : obj->GetChildren()) {
			if (ch) sortedChildren.push_back(ch);
		}
		std::sort(sortedChildren.begin(), sortedChildren.end(), LessByTypeThenName);

		for (auto& ch : sortedChildren) {
			ShowObjectRecursive(ch.get());
		}
		ImGui::TreePop();
	}
	ImGui::PopID();
}

/* ========================================================================
/*  utils
/* ===================================================================== */
bool HierarchyPanel::IsDescendantOf(SceneObject* par, SceneObject* child) {
	if (!child) return false;
	for (auto sp = child->GetParent(); sp; sp = sp->GetParent()) {
		if (sp.get() == par) return true;
	}
	return false;
}

const std::string& HierarchyPanel::GetPanelName() const {
	return panelName_;
}
