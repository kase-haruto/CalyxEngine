#pragma once
/* ========================================================================
/*      include space
/* ===================================================================== */

// engine
#include <Engine/Application/UI/EngineUI/IEngineUI.h>

// c++
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include <externals/imgui/imgui.h>

// forward
class SceneObject;
class SceneObjectLibrary;

namespace CalyxEditor {

	/*-----------------------------------------------------------------------------------------
	 * HierarchyPanel
	 * - ヒエラルキーパネルクラス
	 * - シーン上のオブジェクト階層をツリー表示し、選択・削除・作成を管理
	 *---------------------------------------------------------------------------------------*/
	class HierarchyPanel
		: public IEngineUI {
	private:
		using SelectCB = std::function<void(std::shared_ptr<SceneObject>)>;
		using DeleteCB = std::function<void(std::shared_ptr<SceneObject>)>;
		using CreateCB = std::function<void(std::shared_ptr<SceneObject>)>;
		using RenameCB = std::function<void(std::shared_ptr<SceneObject>, const std::string& newName)>;

	public:
		HierarchyPanel();
		~HierarchyPanel() override = default;

		void Render() override;
		void ShowObjectRecursive(SceneObject* obj);
		bool IsDescendantOf(SceneObject* parent, SceneObject* child);

		void RefreshCache() { cacheDirty_ = true; }

		// accessors -------------------------------------------------------
		const std::string& GetPanelName() const override;

		void SetSceneObjectLibrary(const SceneObjectLibrary* lib) { lib_ = lib; }
		void SetOnObjectSelected(SelectCB cb) { onSelect_ = std::move(cb); }
		void SetOnObjectDelete(DeleteCB cb) { onDelete_ = std::move(cb); }
		void SetOnObjectCreate(CreateCB cb) { onCreate_ = std::move(cb); }
		void SetOnObjectRename(RenameCB cb) { onRename_ = std::move(cb); }

		void SetSelectedObject(std::weak_ptr<SceneObject> wp) { selected_ = wp; }

		const SceneObjectLibrary*  GetSceneObjectLibrary() const { return lib_; }
		std::weak_ptr<SceneObject> GetSelectedObject() const {
			return selected_;
		}

	private:
		// rename
		void BeginRename(SceneObject* obj);
		void CancelRename();
		void CommitRename();
		// render helper
		bool DrawNode(SceneObject* obj);
		bool PassFilterRecursive(SceneObject* obj) const;

	private:
		// runtime state
	private:
		// runtime state
		const SceneObjectLibrary* lib_ = nullptr;
		// キャッシュ: 親オブジェクト(nullptrはルート) -> ソート済み子リスト
		std::unordered_map<const SceneObject*, std::vector<std::shared_ptr<SceneObject>>> sortedCache_;
		bool																			  cacheDirty_ = true;

		std::weak_ptr<SceneObject> selected_;
		std::weak_ptr<SceneObject> renameTarget_;

		SelectCB onSelect_;
		DeleteCB onDelete_;
		CreateCB onCreate_;
		RenameCB onRename_;

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

} // namespace CalyxEditor
