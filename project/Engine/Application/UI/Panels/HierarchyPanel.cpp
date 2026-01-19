#include "HierarchyPanel.h"
/* ========================================================================
/*  include space
/* ===================================================================== */

#include <Data/Engine/Prefab/Serializer/PrefabSerializer.h>
#include <Engine/Application/UI/Panels/InspectorPanel.h>
#include <Engine/Assets/Texture/TextureManager.h>
#include <Engine/Objects/3D/Actor/SceneObject.h>
#include <Engine/Scene/Context/SceneContext.h>
#include <Engine/Objects/3D/Actor/Library/SceneObjectLibrary.h>

// creation headers
#include <Engine/Graphics/Camera/3d/Camera3d.h>
#include <Engine/Objects/LightObject/DirectionalLight.h>
#include <Engine/Objects/LightObject/PointLight.h>
#include <Engine/Objects/3D/Actor/BaseGameObject.h>
#include <Engine/Application/Effects/Particle/Object/ParticleSystemObject.h>

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

		ImGui::Begin(panelName_.c_str(), nullptr, ImGuiWindowFlags_NoDecoration);
		
		// ---------------------------------------------------------------------
		// トップバー (検索 + 設定)
		// ---------------------------------------------------------------------
		{
			ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(4, 4));
			
			// Search Filter
			searchFilter_.Draw("##HierarchyFilter", ImGui::GetContentRegionAvail().x);
			
			ImGui::PopStyleVar();
		}

		lib_ = SceneContext::Current()->GetObjectLibrary();

		if(!lib_) {
			ImGui::Text("SceneObjectLibrary not set.");
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
		}

		// ---------------------------------------------------------------------
		// テーブル (ヘッダー + コンテンツ)
		// ---------------------------------------------------------------------
		static ImGuiTableFlags flags = 
			ImGuiTableFlags_RowBg | 
			ImGuiTableFlags_Resizable | 
			ImGuiTableFlags_Reorderable | 
			ImGuiTableFlags_ScrollY;

		if (ImGui::BeginTable("HierarchyTable", 3, flags)) {
			// カラム設定
			ImGui::TableSetupColumn("Item Label", ImGuiTableColumnFlags_NoHide);
			ImGui::TableSetupColumn("##Visible", ImGuiTableColumnFlags_WidthFixed, 24.0f);
			ImGui::TableSetupColumn("Type", ImGuiTableColumnFlags_WidthFixed, 120.0f);
			
			ImGui::TableHeadersRow();

			// --- root 探索 ---
			std::vector<std::shared_ptr<SceneObject>> roots;
			const auto&								  objects = lib_->GetObjects();
			roots.reserve(objects.size());

			for(const auto& [id, sp] : objects) {
				if(!sp) continue;
				auto parent = sp->GetParent();
				if(!parent || !lib_->Contains(parent)) {
					roots.push_back(sp);
				}
			}

			std::sort(roots.begin(), roots.end(), LessByTypeThenName);

			// --- 描画 ---
			for(auto& sp : roots) {
				ShowObjectRecursive(sp.get());
			}

			ImGui::EndTable();
		}

		// 空白クリックで選択解除 (Tableの外 or Footer)
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
	}

	/* ========================================================================
	/*  recursive UI
	/* ===================================================================== */
	void HierarchyPanel::ShowObjectRecursive(SceneObject* obj) {
		if(!obj) return;

		// フィルタチェック
		if(searchFilter_.IsActive()) {
			if(!PassFilterRecursive(obj)) return;
			ImGui::SetNextItemOpen(true, ImGuiCond_Always);
		}

		// このオブジェクトとその子要素のIDスコープ
		ImGui::PushID(obj);

		// =====================================================================
		// ノード描画 (カスタムウィジェット)
		// =====================================================================
		bool isOpen = DrawNode(obj);

		// =====================================================================
		// Children
		// =====================================================================
		if(isOpen) {
			// Rename中は子を表示しない 
			auto renameSP = renameTarget_.lock();
			bool isRenamingThis = (renaming_ && renameSP.get() == obj);

			if(!isRenamingThis) {

				std::vector<std::shared_ptr<SceneObject>> sortedChildren;
				for(auto& ch : obj->GetChildren()) {
					if(ch) sortedChildren.push_back(ch);
				}

				std::sort(sortedChildren.begin(),
						  sortedChildren.end(),
						  LessByTypeThenName);

				for(auto& ch : sortedChildren) {
					ShowObjectRecursive(ch.get());
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
        
        // タイプのアイコン
		ImTextureID typeTex = nullptr;
		switch(obj->GetObjectType()) {
		case ObjectType::Camera:     typeTex = iconCamera_.tex; break;
		case ObjectType::Light:      typeTex = iconLight_.tex;  break;
		case ObjectType::GameObject: typeTex = iconGameObj_.tex;break;
		case ObjectType::Effect:     typeTex = iconFx_.tex;     break;
		default: break;
		}

        // リネームロジック
		auto renameSP = renameTarget_.lock();
		bool isRenamingThis = (renaming_ && renameSP.get() == obj);
        
		auto selectedPtr = selected_.lock();
		bool isSelected	 = (selectedPtr.get() == obj);
        
        // 16px アイコンを使用
        float iconSize = 16.0f;

		ImGuiTreeNodeFlags flags = 
			ImGuiTreeNodeFlags_OpenOnArrow | 
            ImGuiTreeNodeFlags_SpanAvailWidth |
            ImGuiTreeNodeFlags_SpanFullWidth |
            (isSelected ? ImGuiTreeNodeFlags_Selected : 0) |
            (obj->GetChildren().empty() ? ImGuiTreeNodeFlags_Leaf : 0);

        bool open = false;
        
        if (isRenamingThis) {
             // Rename Mode
             open = ImGui::TreeNodeEx("##rename_dummy", flags | ImGuiTreeNodeFlags_AllowItemOverlap, "");
             ImGui::SameLine();
             
             ImGui::SetKeyboardFocusHere();
             char buf[256];
             snprintf(buf, sizeof(buf), "%s", renameBuf_.c_str());
             
             ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0,0));
             if(ImGui::InputText("##rename", buf, sizeof(buf), ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_AutoSelectAll)) {
                 renameBuf_ = buf;
                 CommitRename();
             }
             ImGui::PopStyleVar();
 
             if (ImGui::IsItemDeactivatedAfterEdit()) {
                 renameBuf_ = buf;
                 CommitRename();
             }
             if (ImGui::IsKeyPressed(ImGuiKey_Escape)) CancelRename();

        } else {
             // 通常モード
             open = ImGui::TreeNodeEx(obj, flags, "");
             
             // インタラクション 
             if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen()) {
                 selected_ = obj->shared_from_this();
                 if(onSelect_) onSelect_(selected_.lock());
             }

             // ドラッグ＆ドロップ
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
                        auto objSP  = obj->shared_from_this();
                        if(lib_->Contains(dragSP) && lib_->Contains(objSP) && !IsDescendantOf(obj, drag)) {
                            drag->SetParent(objSP);
                        }
                    }
                }
                ImGui::EndDragDropTarget();
            }

            // コンテキストメニュー
            if(ImGui::BeginPopupContextItem("SOContext")) { // アイテム上で右クリック
				if(ImGui::BeginMenu("Create Child")) {
					auto createChild = [&](std::shared_ptr<SceneObject> child) {
						child->SetParent(obj->shared_from_this());
						if(onCreate_) onCreate_(child);
					};

					if(ImGui::MenuItem("Empty Scene Object")) createChild(std::make_shared<SceneObject>());
					if(ImGui::MenuItem("Camera")) createChild(std::make_shared<Camera3d>());
					if(ImGui::BeginMenu("Light")) {
						if(ImGui::MenuItem("Directional Light")) createChild(std::make_shared<DirectionalLight>());
						if(ImGui::MenuItem("Point Light")) createChild(std::make_shared<PointLight>());
						ImGui::EndMenu();
					}
					if(ImGui::MenuItem("Mesh Object")) createChild(std::make_shared<BaseGameObject>());
					if(ImGui::MenuItem("Particle System")) createChild(std::make_shared<CalyxEffect::ParticleSystemObject>());

					ImGui::EndMenu();
				}
				ImGui::Separator();
                if(ImGui::MenuItem("Rename")) BeginRename(obj);
                if(ImGui::MenuItem("Delete") && onDelete_) {
                    if(auto sp = obj->shared_from_this()) onDelete_(sp);
                }
				ImGui::Separator();
                if(ImGui::MenuItem("Create Prefab")) {
                    prefabSaveTarget_ = obj;
                    showSavePrefabDlg_ = true;
                }
                ImGui::EndPopup();
            }

            // ショートカット (F2)
            if(isSelected && ImGui::IsWindowFocused() && ImGui::IsKeyPressed(ImGuiKey_F2)) {
                 BeginRename(obj);
            }
             
             // ノード上にアイコンとテキストを描画
             ImGui::SameLine();
             if(typeTex) {
                 ImGui::Image(typeTex, ImVec2(iconSize, iconSize));
                 ImGui::SameLine();
             }
             ImGui::TextUnformatted(obj->GetName().c_str());
        }

		// ---------------------------------------------------------------------
		// カラム 1: 表示切り替え (目)
		// ---------------------------------------------------------------------
        ImGui::TableSetColumnIndex(1);
        
		auto& eyeIcon = obj->IsDrawEnable() ? iconEye_ : iconEyeOff_;
        
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0,0,0,0));
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0,0));
        if(ImGui::ImageButton(eyeIcon.tex, ImVec2(iconSize, iconSize))) {
             obj->SetDrawEnable(!obj->IsDrawEnable());
        }
        ImGui::PopStyleVar();
        ImGui::PopStyleColor();


		// ---------------------------------------------------------------------
		// カラム 2: タイプ情報
		// ---------------------------------------------------------------------
        ImGui::TableSetColumnIndex(2);
        const char* typeName = "Object";
        switch(obj->GetObjectType()) {
            case ObjectType::Camera: typeName = "Camera"; break;
            case ObjectType::Light: typeName = "Light"; break;
            case ObjectType::GameObject: typeName = "Mesh"; break;
            case ObjectType::Effect: typeName = "Effect"; break;
        }
        ImGui::TextDisabled("%s", typeName);

		return open;
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