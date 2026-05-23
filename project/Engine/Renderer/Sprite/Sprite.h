#pragma once

/* engine */
#include <Engine/Foundation/Math/Matrix4x4.h>
#include <Engine/Foundation/Math/Vector2.h>
#include <Engine/Foundation/Math/Vector4.h>
#include <Engine/Graphics/Buffer/DxConstantBuffer.h>
#include <Engine/Graphics/Material.h>
#include <Engine/Graphics/RenderTarget/Detail/RenderTargetDetail.h>
#include <Engine/Objects/Transform/Transform.h>
#include <Engine/Renderer/Mesh/VertexData.h>

/* c++ */
#include <d3d12.h>
#include <wrl.h>

class DirectXCommon;

/*-----------------------------------------------------------------------------------------
 * Sprite
 * - スプライト描画クラス
 * - 2D画像の保持、位置バッファの更新、描画処理を担当
 *---------------------------------------------------------------------------------------*/
class Sprite {
public:
	//===================================================================*/
	//                    public methods
	//===================================================================*/
	/**
	 * \brief コンストラクタ
	 * \param filePath テクスチャファイルのパス
	 */
	Sprite(const std::string& filePath);
	/**
	 * \brief デストラクタ
	 */
	~Sprite();

	/**
	 * \brief 初期化
	 * \param position 座標
	 * \param size サイズ
	 */
	void Initialize(const CalyxEngine::Vector2& position, const CalyxEngine::Vector2& size);
	/**
	 * \brief 初期化 (ウィンドウの中心に配置)
	 */
	void Initialize();

	/**
	 * \brief 更新処理
	 */
	void Update();
	/**
	 * \brief GUI表示
	 */
	void ShowGui();
	/**
	 * \brief 行列の更新
	 */
	void UpdateMatrix();
	/**
	 * \brief トランスフォームの更新
	 */
	void UpdateTransform();
	/**
	 * \brief 描画処理
	 * \param cmdList コマンドリスト
	 */
	void Draw(ID3D12GraphicsCommandList* cmdList);

	// getter
	/**
	 * \brief テクスチャ名を取得
	 * \return テクスチャ名
	 */
	const std::string&					   GetTextureName() const { return path; }
	/**
	 * \brief 描画先レンダリングターゲットを取得
	 * \return レンダリングターゲットタイプ
	 */
	RenderTargetType					   GetTargetRt() const { return targetRT_; }
	/**
	 * \brief 定数バッファリソースを取得
	 * \return リソース
	 */
	Microsoft::WRL::ComPtr<ID3D12Resource> GetConstBuffer() { return vertexResource_; }
	/**
	 * \brief 色を取得 (const)
	 * \return 色
	 */
	const CalyxEngine::Vector4&			   GetColor() const { return materialData_.color; }
	/**
	 * \brief 色を取得
	 * \return 色
	 */
	CalyxEngine::Vector4&					   GetColor() { return materialData_.color; }
	/**
	 * \brief サイズを取得
	 * \return サイズ
	 */
	const CalyxEngine::Vector2&			   GetSize() const { return size; }
	/**
	 * \brief アンカーポイントを取得
	 * \return アンカーポイント
	 */
	const CalyxEngine::Vector2&			   GetAnchorPoint() const { return anchorPoint; }
	/**
	 * \brief 座標を取得
	 * \return 座標
	 */
	const CalyxEngine::Vector2&			   GetPosition() const { return position; }
	/**
	 * \brief UV移動量を取得
	 * \return UV移動量
	 */
	CalyxEngine::Vector2					   GetUvTranslate() const { return CalyxEngine::Vector2(uvTransform.translate.x, uvTransform.translate.y); }
	/**
	 * \brief 切り取り座標(左上)を取得
	 * \return 座標
	 */
	const CalyxEngine::Vector2&			   GetLeftTop() const { return textureLeftTop; }
	/**
	 * \brief 回転角を取得
	 * \return 回転角
	 */
	float								   GetRotation() const { return rotate; }
	/**
	 * \brief UV回転角を取得
	 * \return UV回転角
	 */
	const float							   GetUvRotate() const { return uvTransform.rotate.x; }
	/**
	 * \brief 表示フラグを取得
	 * \return 表示されているか
	 */
	bool								   GetIsVisible() const { return isVisible; }

	// setter
	/**
	 * \brief 回転角を設定
	 * \param rotation 回転角
	 */
	void	   SetRotation(float rotation) { this->rotate = rotation; }
	/**
	 * \brief UV回転角を設定
	 * \param uvRotate UV回転角
	 */
	void	   SetUvRotate(const float uvRotate) { uvTransform.rotate.x = uvRotate; }
	/**
	 * \brief ウィンドウ中央に配置
	 */
	void	   PutWindowCenter();
	/**
	 * \brief 座標を設定
	 * \param newPosition 座標
	 */
	void	   SetPosition(const CalyxEngine::Vector2& newPosition) { this->position = newPosition; }
	/**
	 * \brief UV移動量を設定
	 * \param uvOffset UV移動量
	 */
	void	   SetUvTranslate(const CalyxEngine::Vector2& uvOffset) { CalyxEngine::Vector2(uvTransform.translate.x = uvOffset.x, uvTransform.translate.y = uvOffset.y); }
	/**
	 * \brief 色を設定
	 * \param newColor 色
	 */
	void	   SetColor(const CalyxEngine::Vector4& newColor) { materialData_.color = newColor; }
	/**
	 * \brief サイズを設定
	 * \param newSize サイズ
	 */
	void	   SetSize(const CalyxEngine::Vector2& newSize) { this->size = newSize; }
	/**
	 * \brief 透明度を設定
	 * \param newAlpha 透明度
	 */
	void	   SetAlpha(float newAlpha) { this->materialData_.color.w = newAlpha; }
	/**
	 * \brief アンカーポイントを設定
	 * \param newAnchorPoint アンカーポイント
	 */
	void	   SetAnchorPoint(const CalyxEngine::Vector2& newAnchorPoint) { this->anchorPoint = newAnchorPoint; }
	/**
	 * \brief 切り取り座標(左上)を設定
	 * \param LTop 座標
	 */
	void	   SetLeftTop(const CalyxEngine::Vector2& LTop) { this->textureLeftTop = LTop; }
	/**
	 * \brief 表示フラグを設定
	 * \param is 表示するか
	 */
	void	   SetIsVisible(bool is) { isVisible = is; }
	/**
	 * \brief 描画先レンダリングターゲットを設定
	 * \param targetRt レンダリングターゲットタイプ
	 */
	void	   SetTargetRt(RenderTargetType targetRt) { targetRT_ = targetRt; }
	/**
	 * \brief テクスチャを設定
	 * \param tex テクスチャ名
	 */
	void	   SetTexture(const std::string& tex);
	/**
	 * \brief GPUハンドルからテクスチャを設定
	 * \param newHandle ハンドル
	 */
	const void SetTextureHandle(D3D12_GPU_DESCRIPTOR_HANDLE newHandle);
	/**
	 * \brief UVオフセットを設定
	 * \param offset オフセット
	 */
	void	   SetUvOffset(const CalyxEngine::Vector2& offset);
	/**
	 * \brief UVスケールを設定
	 * \param scale スケール
	 */
	void	   SetUvScale(const CalyxEngine::Vector2& scale);

	/**
	 * \brief 塗りつぶし量を設定 (0.0〜1.0)
	 * \param amt 塗りつぶし量
	 */
	void SetFillAmount(float amt) { materialData_.fillAmount = amt; }
	/**
	 * \brief 塗りつぶしの原点を設定
	 * \param x X座標 (0.0〜1.0)
	 * \param y Y座標 (0.0〜1.0)
	 */
	void SetFillOrigin(float x, float y) { materialData_.fillOrigin = {x, y}; }
	/**
	 * \brief 塗りつぶし手法を設定
	 * \param method 手法
	 */
	void SetFillMethod(int method) { materialData_.fillMethod = method; }

private:
	//===================================================================*/
	//                    private methods
	//===================================================================*/
	/**
	 * \brief バッファを生成
	 */
	void CreateBuffer();
	/**
	 * \brief マッピング
	 */
	void Map();
	/**
	 * \brief インデックスリソースのマッピング
	 */
	void IndexResourceMap();
	/**
	 * \brief 頂点リソースのマッピング
	 */
	void VertexResourceMap();
	/**
	 * \brief 行列リソースのマッピング
	 */
	void TransformResourceMap();
	/**
	 * \brief マテリアルリソースのマッピング
	 */
	void MaterialResourceMap();

private:
	//===================================================================*/
	//                    private member variables
	//===================================================================*/
	EulerTransform transform_{{1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}}; //< 変形情報
	EulerTransform uvTransform{{1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}}; //< UV変形情報
	CalyxEngine::Vector2 position{0.0f, 0.0f}; //< 座標
	float rotate = 0.0f; //< 回転
	CalyxEngine::Vector4 color = {1.0f, 1.0f, 1.0f, 1.0f}; //< 色
	CalyxEngine::Vector2 size = {640.0f, 360.0f}; //< サイズ
	CalyxEngine::Vector2 anchorPoint = {0.0f, 0.0f}; //< アンカーポイント
	CalyxEngine::Vector2 textureLeftTop = {0.0f, 0.0f}; //< テクスチャ左上座標
	CalyxEngine::Vector2 textureSize = {100.0f, 100.0f}; //< テクスチャ切り出しサイズ

	std::string path; //< テクスチャパス

#pragma region リソース
private:
	D3D12_INDEX_BUFFER_VIEW				   indexBufferView{}; //< インデックスバッファビュー
	Microsoft::WRL::ComPtr<ID3D12Resource> vertexResource_; //< 頂点リソース
	Microsoft::WRL::ComPtr<ID3D12Resource> indexResource_; //< インデックスリソース
	Microsoft::WRL::ComPtr<ID3D12Resource> transformResource_; //< 定数バッファリソース（行列用）
	D3D12_VERTEX_BUFFER_VIEW			   vertexBufferViewSprite{}; //< 頂点バッファビュー

	bool				  isVisible		= true; //< 表示フラグ
	CalyxEngine::Matrix4x4* transformData = nullptr; //< 行列データポインタ

	VertexData*					 vertexData = nullptr; //< 頂点データポインタ
	DxConstantBuffer<Material2D> materialCB_; //< マテリアル用定数バッファ
	Material2D					 materialData_; //< マテリアルデータ
	RenderTargetType			 targetRT_ = RenderTargetType::BackBuffer; //< 描画先ターゲット
#pragma endregion

private:
	D3D12_GPU_DESCRIPTOR_HANDLE handle; //< GPUハンドル
};