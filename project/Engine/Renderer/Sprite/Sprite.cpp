#include <Engine/Renderer/Sprite/Sprite.h>
/* ========================================================================
/* include space
/* ===================================================================== */
#include <Engine/Application/System/Environment.h>
#include <Engine/Assets/Texture/TextureManager.h>
#include <Engine/Graphics/Context/GraphicsGroup.h>
#include <Engine/Renderer/Mesh/VertexData.h>

/* math */
#include <Engine/Foundation/Math/Vector3.h>
#include <Engine/Foundation/Utility/Func/CxUtils.h>
#include <Engine/Foundation/Utility/Func/MyFunc.h>

/* c++ */
#include <stdint.h>
/* externals */
#include "Engine/Application/UI/Panels/InspectorPanel.h"
#include "Engine/Assets/Manager/AssetManager.h"
#include "Engine/Foundation/Math/MathUtil.h"

#include <externals/imgui/imgui.h>
#include <Engine/System/Command/EditorCommand/GuiCommand/ImGuiHelper/GuiCmd.h>

Sprite::Sprite(const std::string& filePath) {

	handle = CalyxEngine::AssetManager::GetInstance()->GetTextureManager()->LoadTexture(filePath);

	transform_ = {{1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}};

	path = filePath;

	// リソースの生成
	CreateBuffer();

	// マップ
	Map();

	// トランスフォームと行列の初期化
	UpdateMatrix();
	UpdateTransform();
}

Sprite::~Sprite() {}

void Sprite::Initialize(const CalyxEngine::Vector2& newPosition, const CalyxEngine::Vector2& newSize) {
	this->position		   = newPosition;
	transform_.translate.x = position.x;
	transform_.translate.y = position.y;

	this->size		   = newSize;
	transform_.scale.x = newSize.x;
	transform_.scale.y = newSize.y;
}

void Sprite::Initialize() {
	// Windowの中心に初期化、サイズはデフォルトサイズ
	PutWindowCenter();
}

void Sprite::Update() {
	transform_.translate = {position.x, position.y, 0.0f};
	transform_.rotate	 = {0.0f, 0.0f, rotate};
	transform_.scale	 = {size.x, size.y, 1.0f};

	// アンカー反映（頂点は常時Map済みを前提）
	const float left   = 0.0f - anchorPoint.x;
	const float right  = 1.0f - anchorPoint.x;
	const float top	   = 0.0f - anchorPoint.y;
	const float bottom = 1.0f - anchorPoint.y;

	vertexData[0].position = {left, bottom, 0.0f, 1.0f};
	vertexData[1].position = {left, top, 0.0f, 1.0f};
	vertexData[2].position = {right, bottom, 0.0f, 1.0f};
	vertexData[3].position = {right, top, 0.0f, 1.0f};

	// materialデータの更新
	materialCB_.TransferData(materialData_);

	UpdateMatrix();
	UpdateTransform();
}

void Sprite::ShowGui() {
	if (GuiCmd::BeginSection(CalyxEngine::ParamFilterSection::Object)) {
		GuiCmd::DragFloat2("Position", position, 1.0f);
		GuiCmd::DragFloat2("Size", size, 1.0f);
		GuiCmd::SliderFloat("RotateZ", rotate, -180.0f, 180.0f);
		GuiCmd::DragFloat2("Anchor", anchorPoint, 0.01f, 0.0f, 1.0f);
		GuiCmd::EndSection();
	}
	
	if (GuiCmd::BeginSection(CalyxEngine::ParamFilterSection::Material)) {
		uvTransform.ShowImGui("uvTransform");
		GuiCmd::EndSection();
	}
}

void Sprite::UpdateMatrix() {
	CalyxEngine::Matrix4x4 matWorld	   = CalyxEngine::MakeAffineMatrix(transform_.scale, transform_.rotate, transform_.translate);
	CalyxEngine::Matrix4x4 matView	   = CalyxEngine::Matrix4x4::MakeIdentity();
	CalyxEngine::Matrix4x4 matProjection = CalyxEngine::MakeOrthographicMatrix(0.0f, 0.0f, 1280.0f, 720.0f, 0.0f, 100.0f);
	CalyxEngine::Matrix4x4 wvpMatrix	   = CalyxEngine::Matrix4x4::Multiply(matWorld, CalyxEngine::Matrix4x4::Multiply(matView, matProjection));
	*transformData					   = wvpMatrix;
}

void Sprite::UpdateTransform() {
	///===================================================
	/// UV Transform
	///===================================================
	CalyxEngine::Matrix4x4 uvTransformMatrix = CalyxEngine::MakeScaleMatrix(uvTransform.scale);
	uvTransformMatrix					   = CalyxEngine::Matrix4x4::Multiply(uvTransformMatrix, CalyxEngine::MakeRotateZMatrix(uvTransform.rotate.z));
	uvTransformMatrix					   = CalyxEngine::Matrix4x4::Multiply(uvTransformMatrix, CalyxEngine::MakeTranslateMatrix(uvTransform.translate));
	materialData_.uvTransform			   = uvTransformMatrix;
}

void Sprite::Draw(ID3D12GraphicsCommandList* cmdList) {
	if(!isVisible) return;
	cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	cmdList->SetGraphicsRootDescriptorTable(2, handle);
	cmdList->IASetVertexBuffers(0, 1, &vertexBufferViewSprite);
	cmdList->IASetIndexBuffer(&indexBufferView);
	materialCB_.SetCommand(cmdList, 0);
	cmdList->SetGraphicsRootConstantBufferView(1, transformResource_->GetGPUVirtualAddress());

	cmdList->DrawIndexedInstanced(6, 1, 0, 0, 0);
}

void Sprite::CreateBuffer() {
	ID3D12Device* device = GraphicsGroup::GetInstance()->GetDevice().Get();

	// 頂点用リソース
	vertexResource_ = CreateBufferResource(device, sizeof(VertexData) * 4);

	vertexBufferViewSprite.BufferLocation = vertexResource_->GetGPUVirtualAddress();
	vertexBufferViewSprite.SizeInBytes	  = sizeof(VertexData) * 4;
	vertexBufferViewSprite.StrideInBytes  = sizeof(VertexData);

	// トランスフォーム用リソース
	transformResource_ = CreateBufferResource(device, sizeof(TransformationMatrix));

	// マテリアル用リソース
	materialCB_.Initialize(device);

	// インデックス用リソース
	indexResource_				   = CreateBufferResource(device, sizeof(uint32_t) * 6);
	indexBufferView.BufferLocation = indexResource_->GetGPUVirtualAddress();
	indexBufferView.SizeInBytes	   = sizeof(uint32_t) * 6;
	indexBufferView.Format		   = DXGI_FORMAT_R32_UINT;
}

void Sprite::Map() {
	IndexResourceMap();
	VertexResourceMap();
	TransformResourceMap();
	MaterialResourceMap();
}

void Sprite::IndexResourceMap() {
	uint32_t* indexData = nullptr;
	indexResource_->Map(0, nullptr, reinterpret_cast<void**>(&indexData));

	indexData[0] = 0;
	indexData[1] = 1;
	indexData[2] = 2;
	indexData[3] = 1;
	indexData[4] = 3;
	indexData[5] = 2;

	indexResource_->Unmap(0, nullptr);
}

void Sprite::VertexResourceMap() {
	vertexResource_->Map(0, nullptr, reinterpret_cast<void**>(&vertexData));

	// UV座標の設定
	vertexData[0].texcoord = {0.0f, 1.0f}; // 左下
	vertexData[1].texcoord = {0.0f, 0.0f}; // 左上
	vertexData[2].texcoord = {1.0f, 1.0f}; // 右下
	vertexData[3].texcoord = {1.0f, 0.0f}; // 右上

	// アンカーポイントの反映
	float left	 = 0.0f - anchorPoint.x;
	float right	 = 1.0f - anchorPoint.x;
	float top	 = 0.0f - anchorPoint.y;
	float bottom = 1.0f - anchorPoint.y;

	vertexData[0].position = {left, bottom, 0.0f, 1.0f};  // 左下
	vertexData[1].position = {left, top, 0.0f, 1.0f};	  // 左上
	vertexData[2].position = {right, bottom, 0.0f, 1.0f}; // 右下
	vertexData[3].position = {right, top, 0.0f, 1.0f};	  // 右上

	vertexResource_->Unmap(0, nullptr);
}

void Sprite::TransformResourceMap() {
	transformResource_->Map(0, nullptr, reinterpret_cast<void**>(&transformData));
	*transformData = CalyxEngine::Matrix4x4::MakeIdentity(); // 初期値は単位行列
}

void Sprite::MaterialResourceMap() {
	materialData_.color		  = {1.0f, 1.0f, 1.0f, 1.0f};
	materialData_.uvTransform = CalyxEngine::Matrix4x4::MakeIdentity();
	materialCB_.TransferData(materialData_);
}

void Sprite::PutWindowCenter() {
	CalyxEngine::Vector2 windowCenter = {kWindowWidth * 0.5f, kWindowHeight * 0.5f};
	transform_.translate.x			= windowCenter.x;
	transform_.translate.y			= windowCenter.y;
}

void Sprite::SetUvOffset(const CalyxEngine::Vector2& offset) {
	materialData_.uvOffset = offset;
}

void Sprite::SetUvScale(const CalyxEngine::Vector2& scale) {
	materialData_.uvScale = scale;
}

void Sprite::SetTexture(const std::string& tex) { handle = CalyxEngine::AssetManager::GetInstance()->GetTextureManager()->LoadTexture(tex); }

const void Sprite::SetTextureHandle(D3D12_GPU_DESCRIPTOR_HANDLE newHandle) { handle = newHandle; }