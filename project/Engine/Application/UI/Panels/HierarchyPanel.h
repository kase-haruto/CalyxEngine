#pragma once
/* ========================================================================
/*      include space
/* ===================================================================== */

// engine
#include <Engine/Application/UI/EngineUI/IEngineUI.h>

// c++
#include <vector>
#include <string>
#include <functional>
#include <memory>

// externals
#include <externals/imgui/imgui.h>

// forward
class SceneObject;
class SceneObjectLibrary;

//===================================================================*//
//					HierarchyPanel
//===================================================================*//
class HierarchyPanel
	: public IEngineUI{
private:
	using SelectCB = std::function<void(std::shared_ptr<SceneObject>)>;
	using DeleteCB = std::function<void(std::shared_ptr<SceneObject>)>;
	using CreateCB = std::function<void(std::shared_ptr<SceneObject>)>;

public:
	//===================================================================*/
	//					public methods
	//===================================================================*/
	HierarchyPanel();
	~HierarchyPanel() override = default;

	void Render() override;
	void ShowObjectRecursive(SceneObject* obj);
	bool IsDescendantOf(SceneObject* parent, SceneObject* child);
	const std::string& GetPanelName() const override;

	// accessors -------------------------------------------------------
	void SetSceneObjectLibrary(const SceneObjectLibrary* lib){ lib_ = lib; }
	void SetOnObjectSelected(SelectCB cb){ onSelect_ = std::move(cb); }
	void SetOnObjectDelete(DeleteCB cb){ onDelete_ = std::move(cb); }
	void SetOnObjectCreate(CreateCB cb){ onCreate_ = std::move(cb); }
	void SetSelectedObject(const std::shared_ptr<SceneObject>& sp){ selected_ = sp; }

	const SceneObjectLibrary* GetSceneObjectLibrary() const{ return lib_; }
	std::shared_ptr<SceneObject> GetSelectedObject() const{ return selected_; }

	//===================================================================*/
	//					private methods
	//===================================================================*/
private:
	// runtime state
	const SceneObjectLibrary* lib_ = nullptr;
	std::shared_ptr<SceneObject> selected_;
	SelectCB onSelect_;
	DeleteCB onDelete_;
	CreateCB onCreate_;

	// ぷれふぁbダイアログ
	bool showSavePrefabDlg_ = false;
	bool showLoadPrefabDlg_ = false;
	SceneObject* prefabSaveTarget_ = nullptr;

	// icons
	struct Icon{ ImTextureID tex {}; ImVec2 size {24,24}; };
public:
	Icon iconEye_, iconEyeOff_, iconCamera_, iconLight_, iconGameObj_, iconFx_;

private:
	using IEngineUI::panelName_;
};
