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

class PrimitiveDrawer{
public:
	static PrimitiveDrawer* GetInstance();
	~PrimitiveDrawer() = default;

	void Initialize();
	void Finalize();
	void Render();
	void ClearMesh();

	void DrawGrid();
	void DrawOBB(const CalyxEngine::Vector3& center, const CalyxEngine::Quaternion& rotate, const CalyxEngine::Vector3& size, const CalyxEngine::Vector4& color);
	void DrawAABB(const CalyxEngine::Vector3& minPos, const CalyxEngine::Vector3& maxPos, const CalyxEngine::Vector4& color);
	void DrawSphere(const CalyxEngine::Vector3& center, const float radius = 2.0f, int subdivision = 8, CalyxEngine::Vector4 color ={1.0f,0.0f,0.0f,1.0f});
	void DrawLine3d(const CalyxEngine::Vector3& start, const CalyxEngine::Vector3& end, const CalyxEngine::Vector4& color);
	void DrawBox(const CalyxEngine::Vector3& center, CalyxEngine::Quaternion& rotate, const CalyxEngine::Vector3& size, const CalyxEngine::Vector4& color);
private:
	PrimitiveDrawer() = default;

private:
	std::unique_ptr<LineDrawer> lineDrawer_;
	std::unique_ptr<BoxDrawer> boxDrawer_;
};
