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
#include <vector>

// externals
#include <externals/imgui/imgui.h>

// forward
class SceneObject;
class SceneObjectLibrary;

//===================================================================*//
//					HierarchyPanel
//===================================================================*//
class HierarchyPanel
	: public IEngineUI {
private:
	//===================================================================*/
	//				コールバック
	//===================================================================*/
	using SelectCB = std::function<void(std::shared_ptr<SceneObject>)>;
	using DeleteCB = std::function<void(std::shared_ptr<SceneObject>)>;
	using CreateCB = std::function<void(std::shared_ptr<SceneObject>)>;
	using RenameCB = std::function<void(std::shared_ptr<SceneObject>, const std::string& newName)>;

public:
	//===================================================================*/
	//					public methods
	//===================================================================*/
	HierarchyPanel();
	~HierarchyPanel() override = default;

	void Render() override;

	/// <summary>
	/// オブジェクトの種類表示
	/// </summary>
	/// <param name="obj"></param>
	void ShowObjectRecursive(SceneObject* obj);

	/// <summary>
	/// 親子関係があればtrue
	/// </summary>
	/// <param name="parent"></param>
	/// <param name="child"></param>
	/// <returns></returns>
	bool IsDescendantOf(SceneObject* parent, SceneObject* child);

	// accessors -------------------------------------------------------
	const std::string& GetPanelName() const override;

	void SetSceneObjectLibrary(const SceneObjectLibrary* lib) { lib_ = lib; }
	void SetOnObjectSelected(SelectCB cb) { onSelect_ = std::move(cb); }
	void SetOnObjectDelete(DeleteCB cb) { onDelete_ = std::move(cb); }
	void SetOnObjectCreate(CreateCB cb) { onCreate_ = std::move(cb); }
	void SetObObjectRename(RenameCB cb) { onRename_ = std::move(cb); }

	void SetSelectedObject(const std::shared_ptr<SceneObject>& sp) { selected_ = sp; }

	const SceneObjectLibrary*	 GetSceneObjectLibrary() const { return lib_; }
	std::shared_ptr<SceneObject> GetSelectedObject() const { return selected_; }

private:
	//===================================================================*/
	//					private methods
	//===================================================================*/
	void BeginRename(SceneObject* obj);
	void CancelRename();
	void CommitRename();

private:
	// runtime state
	const SceneObjectLibrary*	 lib_ = nullptr;
	std::shared_ptr<SceneObject> selected_;

	SelectCB onSelect_; //< 選択コールバック
	DeleteCB onDelete_; //< 削除コールバック
	CreateCB onCreate_; //< 作成コールバック
	RenameCB onRename_; //< 再名コールバック

	// ぷれふぁbダイアログ
	bool		 showSavePrefabDlg_ = false;
	bool		 showLoadPrefabDlg_ = false;
	SceneObject* prefabSaveTarget_	= nullptr;

	// リネーム
	bool		 renaming_	   = false;	  //< フラグ
	SceneObject* renameTarget_ = nullptr; //< リネーム大賞
	std::string	 renameBuf_;			  //< 文字列

	// icons
	struct Icon {
		ImTextureID tex{};
		ImVec2		size{24, 24};
	};

public:
	Icon iconEye_, iconEyeOff_, iconCamera_, iconLight_, iconGameObj_, iconFx_;

private:
	using IEngineUI::panelName_;
};
