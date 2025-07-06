#pragma once

/* engine */
#include <Engine/Objects/Transform/Transform.h>
#include <Engine/Foundation/Math/Matrix4x4.h>
#include <Engine/Foundation/Math/Vector2.h>
#include <Engine/Foundation/Math/Vector4.h>
#include <Engine/Graphics/Material.h>
#include <Engine/Renderer/Mesh/VertexData.h>

/* c++ */
#include <d3d12.h>
#include <wrl.h>

using namespace Microsoft::WRL;

class DirectXCommon;

/// <summary>
/// スプライト
/// </summary>
class Sprite{
public:
	Sprite(const std::string& filePath);
	~Sprite();

	/// <summary>
	/// 初期化
	/// </summary>
	void Initialize(const Vector2& position, const Vector2& size);

	/// <summary>
	/// 更新
	/// </summary>
	void Update();

	void UpdateMatrix();

	/// <summary>
	/// 行列の更新
	/// </summary>
	void UpdateMaterix();

	/// <summary>
	/// トランスフォームの更新
	/// </summary>
	void UpdateTransform();

	/// <summary>
	/// 描画
	/// </summary>
	void Draw(ID3D12GraphicsCommandList* cmdList);

	/// <summary>
	/// 定数バッファの作成
	/// </summary>
	void CreateBuffer();

	/// <summary>
	/// マップ
	/// </summary>
	void Map();
	void IndexResourceMap();
	void VertexResourceMap();
	void TransformResourceMap();
	void MaterialResourceMap();

	/// <summary>
	/// 定数バッファの取得
	/// </summary>
	/// <returns></returns>
	ComPtr<ID3D12Resource>GetConstBuffer(){ return vertexResource_; }

	/// <summary>
	/// 回転
	/// </summary>
	/// <param name="rotation"></param>
	void SetRotation(float rotation){ this->rotate = rotation; }
	float GetRotation()const{ return rotate; }

	void SetUvRotate(const float uvRotate){ uvTransform.rotate.x = uvRotate; }
	const float GetUvRotate()const{ return uvTransform.rotate.x; }

	/// <summary>
	/// 座標
	/// </summary>
	/// <param name="position"></param>
	void SetPosition(const Vector2& newPosition){ this->position = newPosition; }
	const Vector2& GetPosition()const{ return position; }

	void SetUvTranslate(const Vector2& uvOffset){ Vector2(uvTransform.translate.x = uvOffset.x, uvTransform.translate.y = uvOffset.y); }
	const Vector2 GetUvTranslate()const{ return Vector2(uvTransform.translate.x, uvTransform.translate.y); }
	/// <summary>
	/// 色
	/// </summary>
	/// <returns></returns>
	const Vector4& GetColor()const{ return materialData_->color; }
	void SetColor(const Vector4& newColor){ materialData_->color = newColor; }

	/// <summary>
	/// サイズ
	/// </summary>
	/// <returns></returns>
	const Vector2& GetSize()const{ return size; }
	void SetSize(const Vector2& newSize){ this->size = newSize; }

	/// <summary>
	/// アンカーポイント
	/// </summary>
	/// <returns></returns>
	const Vector2& GetAnchorPoint()const{ return anchorPoint; }
	void SetAnchorPoint(const Vector2& newAnchorPoint){ this->anchorPoint = newAnchorPoint; }

	/// <summary>
	/// 左上座標
	/// </summary>
	/// <returns></returns>
	const Vector2& GetLeftTop()const{ return textureLeftTop; }
	void SetLeftTop(const Vector2& LTop){ this->textureLeftTop = LTop; }

	const std::string& GetTextureName() const{
		return path;
	}


	// テクスチャハンドルを設定する関数
	const void SetTextureHandle(D3D12_GPU_DESCRIPTOR_HANDLE newHandle);

private:
	EulerTransform transform_ {{1.0f,1.0f,1.0f},{0.0f,0.0f,0.0f},{0.0f,0.0f,0.0f}};
	EulerTransform uvTransform {{1.0f,1.0f,1.0f},{0.0f,0.0f,0.0f},{0.0f,0.0f,0.0f}};
	//座標
	Vector2 position {0.0f,0.0f};
	//回転
	float rotate = 0.0f;
	//色
	Vector4 color = {1.0f,1.0f,1.0f,1.0f};
	//size
	Vector2 size = {640.0f,360.0f};
	//アンカーポイント
	Vector2 anchorPoint = {0.0f,0.0f};
	//テクスチャ左上座標
	Vector2 textureLeftTop = {0.0f,0.0f};
	//テクスチャ切り出しサイズ
	Vector2 textureSize = {100.0f,100.0f};

	std::string path;

#pragma region

private:
	///=============================================================
	///	リソース
	///=============================================================
	//viewの生成
	D3D12_INDEX_BUFFER_VIEW indexBufferView {};
	ComPtr<ID3D12Resource>vertexResource_;
	ComPtr<ID3D12Resource>indexResource_;
	ComPtr<ID3D12Resource>transformResource_;
	D3D12_VERTEX_BUFFER_VIEW vertexBufferViewSprite {};

	//directX関連

	Matrix4x4* transformData = nullptr;

	//マテリアル用のリソース
	ComPtr<ID3D12Resource> materialResource_;
	VertexData* vertexData = nullptr;
	Material2D* materialData_;

#pragma endregion

private:
	///=============================================================
	///	texture
	///=============================================================
	D3D12_GPU_DESCRIPTOR_HANDLE handle;
};
