#pragma once

// engine
#include <Engine/Renderer/Primitive/LineDrawer.h>
#include <Engine/Renderer/Primitive/BoxDrawer.h>

// c++
#include <memory>
#include <vector>

struct CalyxEngine::Vector3;
struct CalyxEngine::Vector4;
struct Matrix4x4;
struct CalyxEngine::Quaternion;

enum class LineDepthMode {
	DepthTest,
	NoDepthTest,
};

/**
 * @brief PrimitiveDrawerの機能を提供するクラスです。
 */
class PrimitiveDrawer{
public:
	static PrimitiveDrawer* GetInstance();
	~PrimitiveDrawer() = default;

	void Initialize();
	void Finalize();
	void Render(bool includeDebugViewOnly = true, LineDepthMode depthMode = LineDepthMode::DepthTest);
	void RenderViewportLines();
	void RenderEffectPreview();
	void ClearMesh();
	void ClearViewportLines();
	void ClearEffectPreview();

	void DrawGrid();
	void DrawOBB(const CalyxEngine::Vector3& center, const CalyxEngine::Quaternion& rotate, const CalyxEngine::Vector3& size, const CalyxEngine::Vector4& color);
	void DrawAABB(const CalyxEngine::Vector3& minPos, const CalyxEngine::Vector3& maxPos, const CalyxEngine::Vector4& color);
	void DrawSphere(const CalyxEngine::Vector3& center, const float radius = 2.0f, int subdivision = 8, CalyxEngine::Vector4 color ={1.0f,0.0f,0.0f,1.0f});
	void DrawCapsule(const CalyxEngine::Vector3& center, const CalyxEngine::Quaternion& rotate, float radius, float height, const CalyxEngine::Vector4& color, int subdivision = 16);
	void DrawLine3d(const CalyxEngine::Vector3& start, const CalyxEngine::Vector3& end, const CalyxEngine::Vector4& color, LineDepthMode depthMode = LineDepthMode::DepthTest);
	void DrawDebugViewLine3d(const CalyxEngine::Vector3& start, const CalyxEngine::Vector3& end, const CalyxEngine::Vector4& color, LineDepthMode depthMode = LineDepthMode::DepthTest);
	void DrawViewportLine3d(const CalyxEngine::Vector3& start, const CalyxEngine::Vector3& end, const CalyxEngine::Vector4& color);
	void DrawBox(const CalyxEngine::Vector3& center, const CalyxEngine::Quaternion& rotate, const CalyxEngine::Vector3& size, const CalyxEngine::Vector4& color);
	void DrawCircle(const CalyxEngine::Vector3& center, const CalyxEngine::Quaternion& rotate, float radiusX, float radiusZ, const CalyxEngine::Vector4& color, int subdivision = 32);
	void DrawCone(const CalyxEngine::Vector3& apex, const CalyxEngine::Quaternion& rotate, float height, float radiusX, float radiusZ, const CalyxEngine::Vector4& color, int subdivision = 32);
	void DrawEffectPreviewSphere(const CalyxEngine::Vector3& center, const float radius = 2.0f, int subdivision = 8, CalyxEngine::Vector4 color ={1.0f,0.0f,0.0f,1.0f});
	void DrawEffectPreviewBox(const CalyxEngine::Vector3& center, const CalyxEngine::Quaternion& rotate, const CalyxEngine::Vector3& size, const CalyxEngine::Vector4& color);
	void DrawEffectPreviewCircle(const CalyxEngine::Vector3& center, const CalyxEngine::Quaternion& rotate, float radiusX, float radiusZ, const CalyxEngine::Vector4& color, int subdivision = 32);
	void DrawEffectPreviewCone(const CalyxEngine::Vector3& apex, const CalyxEngine::Quaternion& rotate, float height, float radiusX, float radiusZ, const CalyxEngine::Vector4& color, int subdivision = 32);
private:
	PrimitiveDrawer() = default;

private:
	std::unique_ptr<LineDrawer> lineDrawer_;
	std::unique_ptr<LineDrawer> lineNoDepthDrawer_;
	std::unique_ptr<LineDrawer> debugViewLineDrawer_;
	std::unique_ptr<LineDrawer> debugViewLineNoDepthDrawer_;
	std::unique_ptr<LineDrawer> viewportLineNoDepthDrawer_;
	std::unique_ptr<BoxDrawer> boxDrawer_;
	std::unique_ptr<LineDrawer> effectPreviewLineDrawer_;
	std::unique_ptr<BoxDrawer> effectPreviewBoxDrawer_;
};
