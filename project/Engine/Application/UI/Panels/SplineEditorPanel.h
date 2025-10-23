#pragma once

// engine
#include <Engine/Application/UI/EngineUI/IEngineUI.h>
#include <Engine/Application/UI/EngineUI/Manipulator.h>
#include <Engine/Foundation/Math/Vector2.h>
#include <Engine/Objects/3D/Geometory/Spline/SplineData.h>
#include <Engine/Objects/Transform/Transform.h>

// std
#include <string>

// fwd
struct Ray;
struct Vector3;

/// <summary>
/// sprineエディタ
/// </summary>
class SplineEditorPanel
	: public IEngineUI {
public:
	SplineEditorPanel();
	~SplineEditorPanel() override;

	/// <summary>
	/// エディタ描画
	/// </summary>
	void Render() override;

	/// <summary>
	/// viewportサイズの同期
	/// </summary>
	/// <param name="pos"></param>
	/// <param name="size"></param>
	void SyncViewportRect(const Vector2& pos, const Vector2& size);

	//--------- accessor -----------------------------------------------------
	SplineData& Data() { return data_; }
	int			GetSelectedIndex() const { return selectedPoint_; }
	void		SetSelectedIndex(int i) { selectedPoint_ = i; }

private:
	/// <summary>
	/// ツールバー描画
	/// </summary>
	void DrawToolbar();

	/// <summary>
	/// 制御点リスト描画
	/// </summary>
	void DrawPointsList();

	/// <summary>
	/// 平面上から見たぷれびゅーを表示
	/// </summary>
	void DrawPreviewXZ();

	/// <summary>
	/// ギズモ更新
	/// </summary>
	void HandleGizmoUpdateAndDraw3D();

	/// <summary>
	/// マウスレイ作成
	/// </summary>
	/// <returns></returns>
	Ray	 MakeMouseRay() const;

	/// <summary>
	/// rayとaabbで判定
	/// </summary>
	/// <param name="ray"></param>
	/// <param name="halfSize"></param>
	/// <param name="outT"></param>
	/// <returns></returns>
	int	 PickPointByRayAABB(const Ray& ray, float halfSize, float& outT) const;

	/// <summary>
	/// planeとの衝突
	/// </summary>
	/// <param name="ray"></param>
	/// <param name="n"></param>
	/// <param name="d"></param>
	/// <param name="out"></param>
	/// <returns></returns>
	bool IntersectPlane(const Ray& ray, const Vector3& n, float d, Vector3& out) const;

private:
	SplineData data_;
	int		   selectedPoint_ = -1;

	std::string currentPath_;
	bool		gizmoEnabled_ = true;

	Vector2 vpPos_{0, 0};
	Vector2 vpSize_{1920, 1080};

	bool	dragging_ = false;
	Vector3 dragPlaneN_{0, 1, 0};
	float	dragPlaneD_ = 0.0f;

	std::unique_ptr<Manipulator> manipulator_;
	WorldTransform				 gizmoTf_;
};