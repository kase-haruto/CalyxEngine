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


void PrimitiveDrawer::DrawLine3d(const CalyxMath::Vector3& start, const CalyxMath::Vector3& end, const CalyxMath::Vector4& color){
	if (lineDrawer_){
		lineDrawer_->DrawLine(start, end, color);
	}
}

void PrimitiveDrawer::DrawBox(const CalyxMath::Vector3& center, CalyxMath::Quaternion& rotate, const CalyxMath::Vector3& size, const CalyxMath::Vector4& color) {
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
		CalyxMath::Vector3 verticalStart(offset, 0.0f, kGridHalfWidth);
		CalyxMath::Vector3 verticalEnd(offset, 0.0f, -kGridHalfWidth);

		CalyxMath::Vector4 verticalColor = (std::abs(offset) < 0.001f) ? CalyxMath::Vector4(0.0f, 1.0f, 0.0f, 1.0f) // X=0 line
			: CalyxMath::Vector4(1.0f, 1.0f, 1.0f, 1.0f);
		DrawLine3d(verticalStart, verticalEnd, verticalColor);

		CalyxMath::Vector3 horizontalStart(-kGridHalfWidth, 0.0f, offset);
		CalyxMath::Vector3 horizontalEnd(kGridHalfWidth, 0.0f, offset);

		CalyxMath::Vector4 horizontalColor = (std::abs(offset) < 0.001f) ? CalyxMath::Vector4(1.0f, 0.0f, 0.0f, 1.0f) // Z=0 line
			: CalyxMath::Vector4(1.0f, 1.0f, 1.0f, 1.0f);
		DrawLine3d(horizontalStart, horizontalEnd, horizontalColor);
	}
}

void PrimitiveDrawer::DrawAABB(const CalyxMath::Vector3& minP, const CalyxMath::Vector3& maxP,
							   const CalyxMath::Vector4& color) {
	// 中心とサイズを計算
	CalyxMath::Vector3 center = (minP + maxP) * 0.5f;
	CalyxMath::Vector3 size = (maxP - minP);

	// OBB は回転付きだが、Identity を渡せば AABB 扱いになる
	CalyxMath::Quaternion identity; // (0,0,0,1)
	identity.MakeIdentity();

	DrawBox(center, identity, size, color); // 既存 API を再利用
}

void PrimitiveDrawer::DrawOBB(const CalyxMath::Vector3& center, const CalyxMath::Quaternion& rotate, const CalyxMath::Vector3& size, const CalyxMath::Vector4& color){
	const uint32_t vertexNum = 8;

	// 各軸の半サイズをクォータニオンで回転
	CalyxMath::Vector3 halfSizeX = CalyxMath::Vector3::Transform({1.0f, 0.0f, 0.0f}, rotate) * (size.x * 0.5f);
	CalyxMath::Vector3 halfSizeY = CalyxMath::Vector3::Transform({0.0f, 1.0f, 0.0f}, rotate) * (size.y * 0.5f);
	CalyxMath::Vector3 halfSizeZ = CalyxMath::Vector3::Transform({0.0f, 0.0f, 1.0f}, rotate) * (size.z * 0.5f);

	// 頂点を計算
	CalyxMath::Vector3 vertices[vertexNum];
	CalyxMath::Vector3 offsets[vertexNum] = {
		{-1, -1, -1}, {-1,  1, -1}, {1, -1, -1}, {1,  1, -1},
		{-1, -1,  1}, {-1,  1,  1}, {1, -1,  1}, {1,  1,  1}
	};

	for (int i = 0; i < vertexNum; ++i){
		CalyxMath::Vector3 localVertex =
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

void PrimitiveDrawer::DrawSphere(const CalyxMath::Vector3& center, const float radius, int subdivision, CalyxMath::Vector4 color){

	// 分割数
	const uint32_t kSubdivision = subdivision;
	const float kLonEvery = 2 * float(std::numbers::pi) / kSubdivision;
	const float kLatEvery = float(std::numbers::pi) / kSubdivision;
	CalyxMath::Vector3 a, b, c, d;

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