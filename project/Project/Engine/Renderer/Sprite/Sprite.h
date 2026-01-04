#pragma once

/* engine */
#include <Engine/Foundation/Math/Matrix4x4.h>
#include <Engine/Foundation/Math/Vector2.h>
#include <Engine/Foundation/Math/Vector4.h>
#include <Engine/Graphics/Material.h>
#include <Engine/Graphics/RenderTarget/Detail/RenderTargetDetail.h>
#include <Engine/Objects/Transform/Transform.h>
#include <Engine/Renderer/Mesh/VertexData.h>

/* c++ */
#include <d3d12.h>
#include <wrl.h>

using namespace Microsoft::WRL;

class DirectXCommon;

class Sprite {
public:
	//===================================================================*/
	//                    public method
	//===================================================================*/
	Sprite(const std::string& filePath);
	~Sprite();

	void Initialize(const Vector2& position, const Vector2& size);
	void Initialize(); // 引数なし初期化初期座標はwindow中心座標

	void Update();
	void ShowGui();
	void UpdateMatrix();
	void UpdateTransform();
	void Draw(ID3D12GraphicsCommandList* cmdList);

	// getter
	const std::string&	   GetTextureName() const { return path; }
	RenderTargetType	   GetTargetRt() const { return targetRT_; }
	ComPtr<ID3D12Resource> GetConstBuffer() { return vertexResource_; }
	const Vector4&		   GetColor() const { return materialData_->color; }
	const Vector2&		   GetSize() const { return size; }
	const Vector2&		   GetAnchorPoint() const { return anchorPoint; }
	const Vector2&		   GetPosition() const { return position; }
	const Vector2&		   GetUvTranslate() const { return Vector2(uvTransform.translate.x, uvTransform.translate.y); }
	const Vector2&		   GetLeftTop() const { return textureLeftTop; }
	float				   GetRotation() const { return rotate; }
	const float			   GetUvRotate() const { return uvTransform.rotate.x; }
	bool				   GetIsVisible() const { return isVisible; }

	// setter
	void	   SetRotation(float rotation) { this->rotate = rotation; }
	void	   SetUvRotate(const float uvRotate) { uvTransform.rotate.x = uvRotate; }
	void	   PutWindowCenter();
	void	   SetPosition(const Vector2& newPosition) { this->position = newPosition; }
	void	   SetUvTranslate(const Vector2& uvOffset) { Vector2(uvTransform.translate.x = uvOffset.x, uvTransform.translate.y = uvOffset.y); }
	void	   SetColor(const Vector4& newColor) { materialData_->color = newColor; }
	void	   SetSize(const Vector2& newSize) { this->size = newSize; }
	void	   SetAnchorPoint(const Vector2& newAnchorPoint) { this->anchorPoint = newAnchorPoint; }
	void	   SetLeftTop(const Vector2& LTop) { this->textureLeftTop = LTop; }
	void	   SetIsVisible(bool is) { isVisible = is; }
	void	   SetTargetRt(RenderTargetType targetRt) { targetRT_ = targetRt; }
	void	   SetTexture(const std::string& tex);
	const void SetTextureHandle(D3D12_GPU_DESCRIPTOR_HANDLE newHandle);

private:
	//===================================================================*/
	//                    private methods
	//===================================================================*/
	void CreateBuffer();
	void Map();
	void IndexResourceMap();
	void VertexResourceMap();
	void TransformResourceMap();
	void MaterialResourceMap();

private:
	//===================================================================*/
	//                    private methods
	//===================================================================*/
	EulerTransform transform_{{1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}};
	EulerTransform uvTransform{{1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}};
	// 座標
	Vector2 position{0.0f, 0.0f};
	// 回転
	float rotate = 0.0f;
	// 色
	Vector4 color = {1.0f, 1.0f, 1.0f, 1.0f};
	// size
	Vector2 size = {640.0f, 360.0f};
	// アンカーポイント
	Vector2 anchorPoint = {0.0f, 0.0f};
	// テクスチャ左上座標
	Vector2 textureLeftTop = {0.0f, 0.0f};
	// テクスチャ切り出しサイズ
	Vector2 textureSize = {100.0f, 100.0f};

	std::string path;

#pragma region

private:
	///=============================================================
	///	リソース
	///=============================================================
	// viewの生成
	D3D12_INDEX_BUFFER_VIEW	 indexBufferView{};
	ComPtr<ID3D12Resource>	 vertexResource_;
	ComPtr<ID3D12Resource>	 indexResource_;
	ComPtr<ID3D12Resource>	 transformResource_;
	D3D12_VERTEX_BUFFER_VIEW vertexBufferViewSprite{};

	// directX関連
	bool	   isVisible	 = true;
	Matrix4x4* transformData = nullptr;

	// マテリアル用のリソース
	ComPtr<ID3D12Resource> materialResource_;
	VertexData*			   vertexData = nullptr;
	Material2D*			   materialData_;
	RenderTargetType	   targetRT_ = RenderTargetType::BackBuffer;
#pragma endregion

private:
	///=============================================================
	///	texture
	///=============================================================
	D3D12_GPU_DESCRIPTOR_HANDLE handle;
};
