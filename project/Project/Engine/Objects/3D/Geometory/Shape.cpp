#include "Shape.h"
/* ========================================================================
/*include Space
/* ===================================================================== */
// engine
#include <Engine/Renderer/Primitive/PrimitiveDrawer.h>
#include <Engine/Foundation/Utility/Func/MyFunc.h>

// c++
#include <numbers>

std::array<Vector3, 8> OBB::GetVertices() const{
	std::array<Vector3, 8> localVertices = {
	Vector3(-size.x, -size.y, -size.z),
	Vector3(-size.x, -size.y,  size.z),
	Vector3(-size.x,  size.y, -size.z),
	Vector3(-size.x,  size.y,  size.z),
	Vector3(size.x, -size.y, -size.z),
	Vector3(size.x, -size.y,  size.z),
	Vector3(size.x,  size.y, -size.z),
	Vector3(size.x,  size.y,  size.z)
	};

	Matrix4x4 rotationMatrix = Quaternion::ToMatrix(rotate); // クォータニオンから回転行列へ変換

	std::array<Vector3, 8> globalVertices;
	for (size_t i = 0; i < localVertices.size(); ++i){
		Vector3 rotated = Vector3::Transform(localVertices[i], rotationMatrix);
		globalVertices[i] = center + rotated;
	}

	return globalVertices;
}


void OBB::Draw(){
	const uint32_t vertexNum = 8;

	Matrix4x4 rotationMatrix = Quaternion::ToMatrix(rotate);

	Vector3 halfSizeX = Vector3::Transform({1.0f, 0.0f, 0.0f}, rotationMatrix) * size.x * 0.5f;
	Vector3 halfSizeY = Vector3::Transform({0.0f, 1.0f, 0.0f}, rotationMatrix) * size.y * 0.5f;
	Vector3 halfSizeZ = Vector3::Transform({0.0f, 0.0f, 1.0f}, rotationMatrix) * size.z * 0.5f;

	Vector3 vertices[vertexNum];
	Vector3 offsets[vertexNum] = {
		{-1, -1, -1}, {-1,  1, -1}, {1, -1, -1}, {1,  1, -1},
		{-1, -1,  1}, {-1,  1,  1}, {1, -1,  1}, {1,  1,  1}
	};

	for (int i = 0; i < vertexNum; ++i){
		Vector3 localVertex =
			offsets[i].x * halfSizeX +
			offsets[i].y * halfSizeY +
			offsets[i].z * halfSizeZ;
		vertices[i] = center + localVertex;
	}

	int edges[12][2] = {
		{0, 1}, {1, 3}, {3, 2}, {2, 0},
		{4, 5}, {5, 7}, {7, 6}, {6, 4},
		{0, 4}, {1, 5}, {2, 6}, {3, 7}
	};

	for (int i = 0; i < 12; ++i){
		PrimitiveDrawer::GetInstance()->DrawLine3d(vertices[edges[i][0]], vertices[edges[i][1]], {1.0f, 0.0f, 0.0f, 1.0f});
	}
}


void Sphere::Draw(int subdivision, Vector4 color){
	// 分割数
	const uint32_t kSubdivision = subdivision; // 分割数を増やして滑らかに
	const float kLonEvery = 2 * float(std::numbers::pi) / kSubdivision;
	const float kLatEvery = float(std::numbers::pi) / kSubdivision;
	Vector3 a, b, c, d;

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
			PrimitiveDrawer::GetInstance()->DrawLine3d(a, c, color);
			PrimitiveDrawer::GetInstance()->DrawLine3d(a, b, color);

			// 緯度方向の線を描画
			PrimitiveDrawer::GetInstance()->DrawLine3d(b, d, color);
			PrimitiveDrawer::GetInstance()->DrawLine3d(c, d, color);
		}
	}
}
