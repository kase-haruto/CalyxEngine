#pragma once

// engine
#include <Engine/Renderer/Primitive/LineDrawer.h>
#include <Engine/Renderer/Primitive/BoxDrawer.h>

// c++
#include <memory>
#include <vector>

struct Vector3;
struct Vector4;
struct Matrix4x4;
struct Quaternion;

class PrimitiveDrawer{
public:
	static PrimitiveDrawer* GetInstance();
	~PrimitiveDrawer() = default;

	void Initialize();
	void Finalize();
	void Render();
	void ClearMesh();

	void DrawGrid();
	void DrawOBB(const Vector3& center, const Quaternion& rotate, const Vector3& size, const Vector4& color);
	void DrawAABB(const Vector3& minPos, const Vector3& maxPos, const Vector4& color);
	void DrawSphere(const Vector3& center, const float radius, int subdivision, Vector4 color);
	void DrawLine3d(const Vector3& start, const Vector3& end, const Vector4& color);
	void DrawBox(const Vector3& center, Quaternion& rotate, const Vector3& size, const Vector4& color);
private:
	PrimitiveDrawer() = default;

private:
	std::unique_ptr<LineDrawer> lineDrawer_;
	std::unique_ptr<BoxDrawer> boxDrawer_;
};
