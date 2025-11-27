#pragma once
// engine
#include <Engine/Application/UI/EngineUI/EditorMenu.h>
#include <Engine/Application/UI/EngineUI/IEngineUI.h>
#include <Engine/Application/UI/EngineUI/Manipulator.h>
#include <Engine/Application/UI/EngineUI/PerformanceOverlay.h>
#include <Engine/Application/UI/EngineUI/Viewport.h>
#include <Engine/Application/UI/Panels/AssetPanel.h>
#include <Engine/Application/UI/Panels/EditorPanel.h>
#include <Engine/Application/UI/Panels/HierarchyPanel.h>
#include <Engine/Application/UI/Panels/InspectorPanel.h>
#include <Engine/Application/UI/Panels/PlaceToolPanel.h>
#include <Engine/Application/UI/Panels/SplineEditorPanel.h>
#include <Engine/Editor/SceneObjectEditor.h>

// c++
#include <memory>
#include <vector>

class BaseEditor;
class SceneContext;
class SceneObject;
class PlaySession;
class BaseCamera;
struct Vector2;
struct Matrix4x4;
struct Ray;

namespace EngineEdit {
enum class EditorMode {
	Edit,
	Game
};
} // namespace EngineEdit

/* ======================================================================== */
/*		レベル編集ツール                                                    */
/* ===================================================================== */
class LevelEditor {
public:
	void Initialize();
	void Update();
	void Render();
	void RenderMenu();
	void ClearSelection();

	// 編集対象 ----------------------------------------------------------------
	void SetSelectedEditor(BaseEditor* editor);
	/// SceneObject の選択（shared_ptr で受けて内部では weak_ptr で保持）
	void SetSelectedObject(const std::shared_ptr<SceneObject>& sp);

	/// シーンへのオブジェクト追加（Prefab / PlaceTool などから呼ばれる）
	void CreateObject(const std::shared_ptr<SceneObject>& obj);
	/// シーンからオブジェクト削除（階層パネルなどから呼ばれる）
	void DeleteObject(const std::shared_ptr<SceneObject>& sp);

	// ビューポート関連 --------------------------------------------------------
	void RenderViewport(ViewportType type, const ImTextureID& tex);
	void SetCameraForViewport(BaseCamera* mainCamera, BaseCamera* debugCamera);

	// パネル取得 --------------------------------------------------------------
	HierarchyPanel* GetHierarchyPanel() const { return hierarchy_.get(); }
	EditorPanel*	GetEditorPanel() const { return editor_.get(); }
	PlaceToolPanel* GetPlaceToolPanel() const { return placeToolPanel_.get(); }

	EngineEdit::EditorMode GetMode() const { return mode_; }
	void				   SetPlaySession(PlaySession* session) { pPlaySesseion_ = session; }

private:
	// マウスピッキング関連 ----------------------------------------------------
	void		 TryPickUnderCursor();
	void		 TryPickObjectFromMouse(const Vector2&	 mouse,
										const Vector2&	 viewportSize,
										const Matrix4x4& view,
										const Matrix4x4& proj);
	SceneObject* PickSceneObjectByRay(const Ray& ray);

	// シーン管理 --------------------------------------------------------------
	void SaveScene();
	void NotifySceneContextChanged();

	// モード切り替え ----------------------------------------------------------
	void EnterGameMode();
	void ExitGameMode();
	void ToggleMode();

	void TogglePanel(IEngineUI* p) {
		if(p) p->SetShow(!p->IsShow());
	}

private:
	// 管理UI
	std::unique_ptr<HierarchyPanel>	   hierarchy_;
	std::unique_ptr<EditorPanel>	   editor_;
	std::unique_ptr<InspectorPanel>	   inspector_;
	std::unique_ptr<SceneObjectEditor> sceneEditor_;
	std::unique_ptr<PlaceToolPanel>	   placeToolPanel_;
	std::unique_ptr<SplineEditorPanel> splineEditor_;
	std::unique_ptr<AssetPanel>		   assetPanel_;
	PlaySession*					   pPlaySesseion_ = nullptr;

	// メニュー
	std::unique_ptr<EditorMenu> menu_; //< エディターメニュー
	EngineEdit::EditorMode		mode_ = EngineEdit::EditorMode::Edit;

	// ビューポート
	std::unique_ptr<Viewport>			mainViewport_;		 //< メインビューポート
	std::unique_ptr<Viewport>			debugViewport_;		 //< デバッグビューポート
	std::unique_ptr<PerformanceOverlay> performanceOverlay_; //< パフォーマンスオーバーレイ

	// 状態
	bool		  lastPlaying_	  = false;
	SceneContext* prevCtx_		  = nullptr;
	BaseEditor*	  selectedEditor_ = nullptr;
	/// SceneObject 選択は weak_ptr で保持（寿命を伸ばさない）
	std::weak_ptr<SceneObject> selectedObject_;

	// Editors メニューに並べるパネル群
	std::vector<IEngineUI*> editorPanels_;
};
