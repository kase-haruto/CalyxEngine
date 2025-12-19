#pragma once

// engine
#include <Engine/Renderer/Primitive/LineDrawer.h>
#include <Engine/Renderer/Primitive/BoxDrawer.h>

// c++
#include <memory>
#include <vector>

struct CxMath::Vector3;
struct CxMath::Vector4;
struct Matrix4x4;
struct CxMath::Quaternion;

class PrimitiveDrawer{
public:
	static PrimitiveDrawer* GetInstance();
	~PrimitiveDrawer() = default;

	void Initialize();
	void Finalize();
	void Render();
	void ClearMesh();

	void DrawGrid();
	void DrawOBB(const CxMath::Vector3& center, const CxMath::Quaternion& rotate, const CxMath::Vector3& size, const CxMath::Vector4& color);
	void DrawAABB(const CxMath::Vector3& minPos, const CxMath::Vector3& maxPos, const CxMath::Vector4& color);
	void DrawSphere(const CxMath::Vector3& center, const float radius = 2.0f, int subdivision = 8, CxMath::Vector4 color ={1.0f,0.0f,0.0f,1.0f});
	void DrawLine3d(const CxMath::Vector3& start, const CxMath::Vector3& end, const CxMath::Vector4& color);
	void DrawBox(const CxMath::Vector3& center, CxMath::Quaternion& rotate, const CxMath::Vector3& size, const CxMath::Vector4& color);
private:
	PrimitiveDrawer() = default;

private:
	std::unique_ptr<LineDrawer> lineDrawer_;
	std::unique_ptr<BoxDrawer> boxDrawer_;
};
