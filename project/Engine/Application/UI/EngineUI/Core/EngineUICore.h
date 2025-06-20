#pragma once

/* ========================================================================
/*	include space
/* ===================================================================== */

// engine
#include <Engine/Application/UI/EngineUI/IEngineUI.h>
#include <Engine/Application/UI/Panels/Controller/PanelController.h>
#include <Engine/Editor/LevelEditor.h>
#include <Engine/Objects/3D/Actor/SceneObject.h>

// c++
#include <d3d12.h>
#include <functional>
#include <memory>
#include <vector>

class EngineUICore {
public:
	//===================================================================*/
	//					public function
	//===================================================================*/
	EngineUICore() = default;			//< コンストラクタ
	~EngineUICore() = default;			//< デストラクタ

	void Initialize();					//< 初期化
	void Update();						//< 更新処理
	void Render();						//< レンダリング

	void AddPanel(std::unique_ptr<IEngineUI> panel);	//< パネル追加

	void NotifySceneContextChanged(class SceneContext* newContext);

	//--------- accessor -----------------------------------------------------
	void SetMainViewportTexture(UINT64 textureID);		//< メインビューポート用のテクスチャを設定
	void SetDebugViewportTexture(UINT64 textureID);		//< デバッグビューポート用のテクスチャを設定
	template<class Panel>
	Panel* GetPanel();
	HierarchyPanel* GetHierarchyPanel() const;
	EditorPanel* GetEditorPanel() const;
	PlaceToolPanel* GetPlaceToolPanel() const;

private:
	//===================================================================*/
	//					private function
	//===================================================================*/

private:
	//===================================================================*/
	//					private variable
	//===================================================================*/
	std::unique_ptr<PanelController> panelController_ = nullptr;
	std::unique_ptr<LevelEditor> levelEditor_ = nullptr;			//< レベルエディタ
	UINT64 mainViewportTextureID_ = 0;
	UINT64 debugViewportTextureID_ = 0;
};

template<class Panel>
Panel* EngineUICore::GetPanel() {
	Panel temp;
	const std::string& name = temp.GetPanelName();
	IEngineUI* base = panelController_->GetPanel(name);
	return dynamic_cast<Panel*>(base);
}
