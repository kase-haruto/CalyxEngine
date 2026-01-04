#include "BoxDrawer.h"

// engine
#include <Engine/Graphics/Context/GraphicsGroup.h>
#include <Engine/Graphics/Camera/Manager/CameraManager.h>
#include <Engine/Graphics/Pipeline/PipelineDesc/Input/VertexLayout.h>
// lib
#include <Engine/Foundation/Math/Vector3.h>
#include <Engine/Foundation/Math/Vector4.h>
#include <Engine/Foundation/Math/Matrix4x4.h>
#include <Engine/Foundation/Math/Quaternion.h>

void BoxDrawer::Initialize() {
	vertexBuffer_.Initialize(GraphicsGroup::GetInstance()->GetDevice(), kMaxBoxes * 36); // 1 box = 12 triangles = 36 vertices
	transformBuffer_.Initialize(GraphicsGroup::GetInstance()->GetDevice(), 1);
}

void BoxDrawer::DrawBox(const Vector3& center, const Quaternion& rotate, const Vector3& size, const Vector4& color){
	if (vertices_.size() + 36 > kMaxBoxes * 36) return;

	Vector3 half = size * 0.5f;

	// ローカル座標の頂点
	Vector3 localVertices[8] = {
		{-half.x, -half.y, -half.z}, // 0
		{+half.x, -half.y, -half.z}, // 1
		{+half.x, +half.y, -half.z}, // 2
		{-half.x, +half.y, -half.z}, // 3
		{-half.x, -half.y, +half.z}, // 4
		{+half.x, -half.y, +half.z}, // 5
		{+half.x, +half.y, +half.z}, // 6
		{-half.x, +half.y, +half.z}  // 7
	};

	// 回転と平行移動を適用
	Vector3 worldVertices[8];
	for (int i = 0; i < 8; ++i){
		Vector3 rotated = Vector3::Transform(localVertices[i], rotate);
		worldVertices[i] = rotated + center;
	}

	const int faceIndices[][6] = {
		{0, 1, 2, 0, 2, 3}, // -Z
		{5, 4, 7, 5, 7, 6}, // +Z
		{4, 0, 3, 4, 3, 7}, // -X
		{1, 5, 6, 1, 6, 2}, // +X
		{3, 2, 6, 3, 6, 7}, // +Y
		{4, 5, 1, 4, 1, 0}  // -Y
	};

	for (const auto& face : faceIndices){
		for (int i = 0; i < 6; ++i){
			vertices_.emplace_back(VertexPosColor {worldVertices[face[i]], color});
		}
	}
}


void BoxDrawer::Render() {
	if (vertices_.empty()) return;

	auto cmdList = GraphicsGroup::GetInstance()->GetCommandList();
	cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_LINELIST);

	// 頂点バッファ更新
	vertexBuffer_.TransferVectorData(vertices_);
	vertexBuffer_.SetCommand(cmdList);

	// WVP行列
	Matrix4x4 identity = Matrix4x4::MakeIdentity();
	TransformationMatrix wvpData;
	wvpData.world = identity;
	wvpData.WorldInverseTranspose = Matrix4x4::Transpose(Matrix4x4::Inverse(identity));
	transformBuffer_.TransferData(wvpData);
	transformBuffer_.SetCommand(cmdList, 0);

	cmdList->DrawInstanced(static_cast<UINT>(vertices_.size()), 1, 0, 0);
}

void BoxDrawer::Clear() {
	vertices_.clear();
}
