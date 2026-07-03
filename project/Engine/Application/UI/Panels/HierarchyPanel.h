#pragma once
/* ========================================================================
/*      include space
/* ===================================================================== */

// engine
#include <Engine/Application/UI/Panels/HierarchyActions.h>
#include <Engine/Application/UI/EngineUI/IEngineUI.h>
#include <Engine/Application/UI/Panels/HierarchyTreeCache.h>

// c++
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <externals/imgui/imgui.h>

// forward
class BaseGameObject;
class SceneObject;
class SceneObjectLibrary;
enum class ObjectType;

namespace CalyxEngine {

	/*-----------------------------------------------------------------------------------------
	 * HierarchyPanel
	 * - ヒエラルキーパネルクラス
	 * - シーン上のオブジェクト階層をツリー表示し、選択・削除・作成を管理
	 *---------------------------------------------------------------------------------------*/
	class HierarchyPanel
		: public IEngineUI {
	private:
		using SelectCB = CallbackHierarchyActions::SelectCB;
		using ObjectCB = CallbackHierarchyActions::ObjectCB;
		using RenameCB = CallbackHierarchyActions::RenameCB;

	public:
		HierarchyPanel();
		~HierarchyPanel() override = default;

		void Render() override;
		void ShowObjectRecursive(SceneObject* obj, bool forceShow = false, bool highlightBoneChild = false);
		bool IsDescendantOf(SceneObject* parent, SceneObject* child);

		void RefreshCache() { treeCache_.MarkDirty(); }

		// accessors -------------------------------------------------------
		const std::string& GetPanelName() const override;

		void SetSceneObjectLibrary(const SceneObjectLibrary* lib) { lib_ = lib; }
		void SetActions(IHierarchyActions* actions) { actions_ = actions ? actions : &callbackActions_; }
		void SetOnObjectSelected(SelectCB cb) { callbackActions_.SetOnObjectSelected(std::move(cb)); }
		void SetOnObjectDelete(ObjectCB cb) { callbackActions_.SetOnObjectDelete(std::move(cb)); }
		void SetOnObjectCreate(ObjectCB cb) { callbackActions_.SetOnObjectCreate(std::move(cb)); }
		void SetOnObjectRename(RenameCB cb) { callbackActions_.SetOnObjectRename(std::move(cb)); }
		void SetOnApplyPrefabOverrides(ObjectCB cb) { callbackActions_.SetOnApplyPrefabOverrides(std::move(cb)); }
		void SetOnObjectFocused(ObjectCB cb) { callbackActions_.SetOnObjectFocused(std::move(cb)); }

		void SetSelectedObject(std::weak_ptr<SceneObject> wp) {
			selected_ = wp;
			selectedObjects_.clear();
			if(auto sp = wp.lock()) {
				selectedObjects_.push_back(sp);
			}
		}
		void SetSelectedObjects(const std::vector<std::shared_ptr<SceneObject>>& objects);

		const SceneObjectLibrary*  GetSceneObjectLibrary() const { return lib_; }
		std::weak_ptr<SceneObject> GetSelectedObject() const {
			return selected_;
		}
		std::vector<std::weak_ptr<SceneObject>> GetSelectedObjects() const {
			return selectedObjects_;
		}

	private:
		// rename
		void BeginRename(SceneObject* obj);
		void CancelRename();
		void CommitRename();
		// render helper
		bool DrawNode(SceneObject* obj, bool highlightBoneChild = false);
		bool DrawBoneRootNode(BaseGameObject* owner, const std::vector<std::string>& boneNames);
		void DrawBoneNode(BaseGameObject* owner, const std::string& boneName);
		std::vector<SceneObject*> GetBoneAttachedObjects(BaseGameObject* owner, const std::string& boneName) const;
		bool IsBoneParentedObject(SceneObject* object) const;
		void HandleNodeSelectionClick(SceneObject* obj);
		ImTextureID GetTypeIcon(ObjectType type) const;
		const char* GetTypeLabel(ObjectType type) const;
		bool PassFilterRecursive(SceneObject* obj) const;
		bool IsSelected(SceneObject* obj) const;
		void ClearBoneParentBindings(SceneObject& target);
		void MarkBoneOwnerExpanded(BaseGameObject* owner);
		void RestoreCollapsedBoneDrawStates();
		void ForgetBoneDrawState(SceneObject* object);

	private:
		// runtime state
	private:
		// runtime state
		const SceneObjectLibrary* lib_ = nullptr;
		HierarchyTreeCache treeCache_;

		std::weak_ptr<SceneObject> selected_;
		std::vector<std::weak_ptr<SceneObject>> selectedObjects_;
		// A hierarchy press becomes a selection only when released without starting a drag.
		std::weak_ptr<SceneObject> pendingSelection_;
		bool pendingSelectionDragged_ = false;
		std::weak_ptr<SceneObject> renameTarget_;
		std::unordered_map<BaseGameObject*, bool> autoSkeletonDrawPrevious_;
		std::unordered_set<BaseGameObject*> expandedBoneOwnersThisFrame_;

		CallbackHierarchyActions callbackActions_;
		IHierarchyActions*		actions_ = &callbackActions_;

		// prefab dialog
		bool		 showSavePrefabDlg_ = false;
		bool		 showLoadPrefabDlg_ = false;
		SceneObject* prefabSaveTarget_	= nullptr;

		// rename buffer
		bool		renaming_ = false;
		std::string renameBuf_;

		// search
		ImGuiTextFilter searchFilter_;

		// icons
		struct Icon {
			ImTextureID tex{};
			ImVec2		size{24, 24};
		};

	public:
		Icon iconEye_, iconEyeOff_, iconCamera_, iconLight_, iconGameObj_, iconFx_;
		int	 rowIndex_ = 0;

	private:
		using IEngineUI::panelName_;
	};

} // namespace CalyxEngine
