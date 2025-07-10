#pragma once
#include <Engine/Editor/SceneObjectEditor.h>
#include <Engine/Application/UI/Panels/EditorPanel.h>
#include <Engine/Application/UI/Panels/HierarchyPanel.h>
#include <Engine/Application/UI/Panels/InspectorPanel.h>
#include <Engine/Application/UI/Panels/PlaceToolPanel.h>
#include <Engine/Application/UI/EngineUI/Manipulator.h>
#include <Engine/Application/UI/EngineUI/Viewport.h>
#include <Engine/Application/UI/EngineUI/EditorMenu.h>
#include <Engine/Application/UI/EngineUI/PerformanceOverlay.h>

//c++
#include <memory>

class BaseEditor;
class SceneContext;
class SceneObject;

class LevelEditor{
public:
	void Initialize();
	void Update();
	void Render();

	// 編集対象
	void SetSelectedEditor(BaseEditor* editor);
	void SetSelectedObject(SceneObject* object);
	void CreateObject(std::shared_ptr<SceneObject> obj);
	void DeleteObject(SceneObject* object);
	void RenderViewport(ViewportType type, const ImTextureID& tex);

	HierarchyPanel* GetHierarchyPanel() const{ return hierarchy_.get(); }
	EditorPanel* GetEditorPanel() const{ return editor_.get(); }
	PlaceToolPanel* GetPlaceToolPanel() const{ return placeToolPanel_.get(); }
	void NotifySceneContextChanged(SceneContext* newContext);

private:
	void TryPickUnderCursor();
	void TryPickObjectFromMouse(const Vector2& mouse, const Vector2& viewportSize, const Matrix4x4& view, const Matrix4x4& proj);
	SceneObject* PickSceneObjectByRay(const struct Ray& ray);

private:
	void SaveScene();
	void ClearSelection(){
		selectedEditor_ = nullptr;
		selectedObject_ = nullptr;
		hierarchy_->SetSelectedObject(nullptr);
		inspector_->SetSelectedObject(nullptr);
		sceneEditor_->ClearSelection();
	}

private:
	// 管理UI
	std::unique_ptr<HierarchyPanel> hierarchy_;
	std::unique_ptr<EditorPanel> editor_;
	std::unique_ptr<InspectorPanel> inspector_;
	std::unique_ptr<SceneObjectEditor> sceneEditor_;
	std::unique_ptr<PlaceToolPanel> placeToolPanel_;
	// メニュー
	std::unique_ptr<EditorMenu> menu_;				//< エディターメニュー

	// ビューポート
	std::unique_ptr<Viewport> mainViewport_;				//< メインビューポート
	std::unique_ptr<Viewport> debugViewport_;				//< デバッグビューポート
	std::unique_ptr<PerformanceOverlay> performanceOverlay_;	//< パフォーマンスオーバーレイ

	// 状態
	BaseEditor* selectedEditor_ = nullptr;
	SceneObject* selectedObject_ = nullptr;
	SceneContext* pSceneContext_ = nullptr;	//< 現在のシーンコンテキスト
};