#include "PrimitiveDrawer.h"

#include <Engine/Foundation/Math/Vector3.h>
#include <Engine/Foundation/Math/Vector4.h>
#include <Engine/Foundation/Math/Matrix4x4.h>
#include <Engine/Foundation/Utility/Func/MyFunc.h>

#include <Engine/Graphics/Context/GraphicsGroup.h>

#include <cmath>
#include <numbers>

PrimitiveDrawer* PrimitiveDrawer::GetInstance(){
	static PrimitiveDrawer instance;
	return &instance;
}

void PrimitiveDrawer::Initialize(){
	lineDrawer_ = std::make_unique<LineDrawer>();
	lineDrawer_->Initialize();

	boxDrawer_ = std::make_unique<BoxDrawer>();
	boxDrawer_->Initialize();
}

void PrimitiveDrawer::Finalize(){
	if (lineDrawer_){
		lineDrawer_->Clear();
	}
	lineDrawer_.reset();

	if (boxDrawer_) {
		boxDrawer_->Clear();
	}
	boxDrawer_.reset();
}


void PrimitiveDrawer::DrawLine3d(const CalyxEngine::Vector3& start, const CalyxEngine::Vector3& end, const CalyxEngine::Vector4& color){
	if (lineDrawer_){
		lineDrawer_->DrawLine(start, end, color);
	}
}

void PrimitiveDrawer::DrawBox(const CalyxEngine::Vector3& center, CalyxEngine::Quaternion& rotate, const CalyxEngine::Vector3& size, const CalyxEngine::Vector4& color) {
	if (boxDrawer_) {
		boxDrawer_->DrawBox(center, rotate,size, color);
	}
}

void PrimitiveDrawer::DrawGrid(){
	const uint32_t kSubdivision = 32;
	const float kGridHalfWidth = 32.0f;
	const float kGridEvery = (kGridHalfWidth * 2.0f) / float(kSubdivision);

	for (uint32_t index = 0; index <= kSubdivision; ++index){
		float offset = -kGridHalfWidth + index * kGridEvery;

		// --- 縦線（Z軸方向） ---
		CalyxEngine::Vector3 verticalStart(offset, 0.0f, kGridHalfWidth);
		CalyxEngine::Vector3 verticalEnd(offset, 0.0f, -kGridHalfWidth);

		CalyxEngine::Vector4 verticalColor = (std::abs(offset) < 0.001f) ? CalyxEngine::Vector4(0.0f, 1.0f, 0.0f, 1.0f) // X=0 line
			: CalyxEngine::Vector4(1.0f, 1.0f, 1.0f, 1.0f);
		DrawLine3d(verticalStart, verticalEnd, verticalColor);

		CalyxEngine::Vector3 horizontalStart(-kGridHalfWidth, 0.0f, offset);
		CalyxEngine::Vector3 horizontalEnd(kGridHalfWidth, 0.0f, offset);

		CalyxEngine::Vector4 horizontalColor = (std::abs(offset) < 0.001f) ? CalyxEngine::Vector4(1.0f, 0.0f, 0.0f, 1.0f) // Z=0 line
			: CalyxEngine::Vector4(1.0f, 1.0f, 1.0f, 1.0f);
		DrawLine3d(horizontalStart, horizontalEnd, horizontalColor);
	}
}

void PrimitiveDrawer::DrawAABB(const CalyxEngine::Vector3& minP, const CalyxEngine::Vector3& maxP,
							   const CalyxEngine::Vector4& color) {
	// 中心とサイズを計算
	CalyxEngine::Vector3 center = (minP + maxP) * 0.5f;
	CalyxEngine::Vector3 size = (maxP - minP);

	// OBB は回転付きだが、Identity を渡せば AABB 扱いになる
	CalyxEngine::Quaternion identity; // (0,0,0,1)
	identity.MakeIdentity();

	DrawBox(center, identity, size, color); // 既存 API を再利用
}

void PrimitiveDrawer::DrawOBB(const CalyxEngine::Vector3& center, const CalyxEngine::Quaternion& rotate, const CalyxEngine::Vector3& size, const CalyxEngine::Vector4& color){
	const uint32_t vertexNum = 8;

	// 各軸の半サイズをクォータニオンで回転
	CalyxEngine::Vector3 halfSizeX = CalyxEngine::Vector3::Transform({1.0f, 0.0f, 0.0f}, rotate) * (size.x * 0.5f);
	CalyxEngine::Vector3 halfSizeY = CalyxEngine::Vector3::Transform({0.0f, 1.0f, 0.0f}, rotate) * (size.y * 0.5f);
	CalyxEngine::Vector3 halfSizeZ = CalyxEngine::Vector3::Transform({0.0f, 0.0f, 1.0f}, rotate) * (size.z * 0.5f);

	// 頂点を計算
	CalyxEngine::Vector3 vertices[vertexNum];
	CalyxEngine::Vector3 offsets[vertexNum] = {
		{-1, -1, -1}, {-1,  1, -1}, {1, -1, -1}, {1,  1, -1},
		{-1, -1,  1}, {-1,  1,  1}, {1, -1,  1}, {1,  1,  1}
	};

	for (int i = 0; i < vertexNum; ++i){
		CalyxEngine::Vector3 localVertex =
			offsets[i].x * halfSizeX +
			offsets[i].y * halfSizeY +
			offsets[i].z * halfSizeZ;
		vertices[i] = center + localVertex;
	}

	// 辺を描画
	int edges[12][2] = {
		{0, 1}, {1, 3}, {3, 2}, {2, 0},
		{4, 5}, {5, 7}, {7, 6}, {6, 4},
		{0, 4}, {1, 5}, {2, 6}, {3, 7}
	};

	for (int i = 0; i < 12; ++i){
		DrawLine3d(vertices[edges[i][0]], vertices[edges[i][1]], color);
	}
}

void PrimitiveDrawer::DrawSphere(const CalyxEngine::Vector3& center, const float radius, int subdivision, CalyxEngine::Vector4 color){

	// 分割数
	const uint32_t kSubdivision = subdivision;
	const float kLonEvery = 2 * float(std::numbers::pi) / kSubdivision;
	const float kLatEvery = float(std::numbers::pi) / kSubdivision;
	CalyxEngine::Vector3 a, b, c, d;

	// 緯度方向に分割
	for (uint32_t latIndex = 0; latIndex < kSubdivision; ++latIndex){
		float lat = -float(std::numbers::pi) / 2.0f + kLatEvery * latIndex;
		float nextLat = lat + kLatEvery;

		// 経度方向に分割
		for (uint32_t lonIndex = 0; lonIndex < kSubdivision; ++lonIndex){
			float lon = lonIndex * kLonEvery;
			float nextLon = lon + kLonEvery;

			// 点の座標を計算
			a.x = radius * (std::cos(lat) * std::cos(lon)) + center.x;
			a.y = radius * std::sin(lat) + center.y;
			a.z = radius * (std::cos(lat) * std::sin(lon)) + center.z;

			b.x = radius * (std::cos(nextLat) * std::cos(lon)) + center.x;
			b.y = radius * std::sin(nextLat) + center.y;
			b.z = radius * (std::cos(nextLat) * std::sin(lon)) + center.z;

			c.x = radius * (std::cos(lat) * std::cos(nextLon)) + center.x;
			c.y = radius * std::sin(lat) + center.y;
			c.z = radius * (std::cos(lat) * std::sin(nextLon)) + center.z;

			d.x = radius * (std::cos(nextLat) * std::cos(nextLon)) + center.x;
			d.y = radius * std::sin(nextLat) + center.y;
			d.z = radius * (std::cos(nextLat) * std::sin(nextLon)) + center.z;

			// 経度方向の線を描画
			DrawLine3d(a, c, color);
			DrawLine3d(a, b, color);

			// 緯度方向の線を描画
			DrawLine3d(b, d, color);
			DrawLine3d(c, d, color);
		}
	}

}

void PrimitiveDrawer::Render(){
#if defined(_DEBUG) || defined(DEVELOP)
	if (lineDrawer_) {
		lineDrawer_->Render();
	}

	if (boxDrawer_) {
		boxDrawer_->Render();
	}
#endif // _DEBUG


	
}

void PrimitiveDrawer::ClearMesh(){
	if (lineDrawer_){
		lineDrawer_->Clear();
	}

	if (boxDrawer_) {
		boxDrawer_->Clear();
	}
}