#include "LineDrawer.h"

#include <Engine/Foundation/Math/Vector3.h>
#include <Engine/Foundation/Math/Vector4.h>
#include <Engine/Foundation/Math/Matrix4x4.h>
#include <Engine/Graphics/Context/GraphicsGroup.h>
#include <Engine/Graphics/Camera/Manager/CameraManager.h>
#include <Engine/Graphics/Pipeline/PipelineDesc/Input/VertexLayout.h>

void LineDrawer::Initialize(){
	vertexBuffer_.Initialize(GraphicsGroup::GetInstance()->GetDevice(), kMaxLines * 2);
	transformBuffer_.Initialize(GraphicsGroup::GetInstance()->GetDevice(), 1);
}

void LineDrawer::DrawLine(const CalyxEngine::Vector3& start, const CalyxEngine::Vector3& end, const CalyxEngine::Vector4& color){
	if (vertices_.size() / 2 >= kMaxLines) return;

	vertices_.emplace_back(VertexPosColor {start, color});
	vertices_.emplace_back(VertexPosColor {end, color});
}

void LineDrawer::Render(){
	if (vertices_.empty()) return;

	auto cmdList = GraphicsGroup::GetInstance()->GetCommandList();
	

	cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_LINELIST);

	// 頂点バッファ更新
	vertexBuffer_.TransferVectorData(vertices_);
	vertexBuffer_.SetCommand(cmdList);

	// WVP定数バッファ更新
	CalyxEngine::Matrix4x4 identity = CalyxEngine::Matrix4x4::MakeIdentity();

	TransformationMatrix wvpData;
	wvpData.world = identity;
	wvpData.WorldInverseTranspose = CalyxEngine::Matrix4x4::Transpose(CalyxEngine::Matrix4x4::Inverse(identity));

	transformBuffer_.TransferData(wvpData);
	transformBuffer_.SetCommand(cmdList, 0);

	// 描画
	cmdList->DrawInstanced(static_cast< UINT >(vertices_.size()), 1, 0, 0);
}


void LineDrawer::Clear(){
	vertices_.clear();
}
