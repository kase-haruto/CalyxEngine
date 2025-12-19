#pragma once

// engine
#include <Engine/Renderer/Primitive/LineDrawer.h>
#include <Engine/Renderer/Primitive/BoxDrawer.h>

// c++
#include <memory>
#include <vector>

struct CalyxMath::Vector3;
struct CalyxMath::Vector4;
struct Matrix4x4;
struct CalyxMath::Quaternion;

class PrimitiveDrawer{
public:
	static PrimitiveDrawer* GetInstance();
	~PrimitiveDrawer() = default;

	void Initialize();
	void Finalize();
	void Render();
	void ClearMesh();

	void DrawGrid();
	void DrawOBB(const CalyxMath::Vector3& center, const CalyxMath::Quaternion& rotate, const CalyxMath::Vector3& size, const CalyxMath::Vector4& color);
	void DrawAABB(const CalyxMath::Vector3& minPos, const CalyxMath::Vector3& maxPos, const CalyxMath::Vector4& color);
	void DrawSphere(const CalyxMath::Vector3& center, const float radius = 2.0f, int subdivision = 8, CalyxMath::Vector4 color ={1.0f,0.0f,0.0f,1.0f});
	void DrawLine3d(const CalyxMath::Vector3& start, const CalyxMath::Vector3& end, const CalyxMath::Vector4& color);
	void DrawBox(const CalyxMath::Vector3& center, CalyxMath::Quaternion& rotate, const CalyxMath::Vector3& size, const CalyxMath::Vector4& color);
private:
	PrimitiveDrawer() = default;

private:
	std::unique_ptr<LineDrawer> lineDrawer_;
	std::unique_ptr<BoxDrawer> boxDrawer_;
};
