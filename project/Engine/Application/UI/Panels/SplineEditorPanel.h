#pragma once
#include <Engine/Application/UI/EngineUI/IEngineUI.h>
#include <Engine/Objects/3D/Geometory/Spline/SplineData.h>
#include <Engine/Foundation/Math/Vector2.h>
#include <string>

class SceneContext;
struct Ray;
struct Vector3;

class SplineEditorPanel 
	: public IEngineUI {
public:
	SplineEditorPanel() : IEngineUI("SplineEditor") {}
	~SplineEditorPanel() override = default;

	void SetSceneContext(SceneContext* ctx) { ctx_ = ctx; }
	void Render() override;

	// debug ビューポートの矩形（ImGui座標）を同期：Viewportそのものは渡さない
	void SyncViewportRect(const Vector2& pos, const Vector2& size) {
		vpPos_ = pos; vpSize_ = size;
	}

	// データ & 選択
	SplineData& Data() { return data_; }
	int  GetSelectedIndex() const { return selectedPoint_; }
	void SetSelectedIndex(int i) { selectedPoint_ = i; }

private:
	// UI 部分
	void DrawToolbar();
	void DrawPointsList();
	void DrawPreviewXZ();

	// ギズモ（パネル内で完結）
	void HandleGizmoUpdateAndDraw3D();
	Ray  MakeMouseRay() const;
	int  PickPointByRayAABB(const Ray& ray, float halfSize, float& outT) const;
	bool IntersectPlane(const Ray& ray, const Vector3& n, float d, Vector3& out) const;

private:
	SceneContext* ctx_ = nullptr;

	SplineData data_;
	int selectedPoint_ = -1;

	std::string currentPath_;
	bool gizmoEnabled_ = true; // デフォルトONにしておく

	// 同期された debug ビューの矩形
	Vector2 vpPos_{ 0,0 };
	Vector2 vpSize_{ 1920,1080 };

	// ドラッグ状態
	bool    dragging_ = false;
	Vector3 dragPlaneN_{ 0,1,0 };
	float   dragPlaneD_ = 0.0f;
};
