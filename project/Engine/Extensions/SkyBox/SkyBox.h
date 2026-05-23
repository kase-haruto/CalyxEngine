#pragma once

/* ========================================================================
/* include space
/* ===================================================================== */

#include <Engine/Renderer/Mesh/VertexData.h>
#include <Engine/Objects/Transform/Transform.h>
#include <Engine/Graphics/Buffer/DxIndexBuffer.h>
#include <Engine/Graphics/Buffer/DxVertexBuffer.h>
#include <Engine/Graphics/Buffer/DxConstantBuffer.h>
#include <Engine/Objects/3D/Actor/SceneObject.h>

#include <string>
#include <array> 
#include <functional>
#include <optional>

/*-----------------------------------------------------------------------------------------
 * SkyBox
 * - スカイボックス描画クラス
 * - 背景としての巨大な立方体の描画、およびテクスチャ管理を担当
 *---------------------------------------------------------------------------------------*/
class SkyBox :
	public SceneObject{
public:
	//===================================================================*/
	//                   public methods
	//===================================================================*/
	/**
	 * \brief コンストラクタ
	 * \param fileName テクスチャファイル名
	 * \param objectName オブジェクト名
	 */
	SkyBox(const std::string& fileName,
		   std::optional<std::string> objectName);

	/**
	 * \brief デストラクタ
	 */
	~SkyBox()override = default;

	/**
	 * \brief 初期化
	 */
	void Initialize();

	/**
	 * \brief ImGui表示
	 */
	void ShowGui();

	/**
	 * \brief 更新処理
	 * \param dt デルタタイム
	 */
	void Update(float dt)override;

	/**
	 * \brief 常時更新処理
	 * \param dt デルタタイム
	 */
	void AlwaysUpdate(float dt)override;

	/**
	 * \brief 描画処理
	 * \param cmd コマンドリスト
	 */
	void Draw(ID3D12GraphicsCommandList* cmd);

	/**
	 * \brief ワールド変換を取得
	 * \return ワールド変換
	 */
	const WorldTransform& GetWorldTransform()const;

	//===================================================================*/
	//                   config
	//===================================================================*/
	/**
	 * \brief タイプ名を取得
	 * \return タイプ名
	 */
	std::string_view GetObjectClassName() const override{return "SkyBox";}

private:
	//===================================================================*/
	//                    private member variables
	//===================================================================*/
	std::array<VertexData, 24> vertices_; //< 頂点データ配列
	std::array<uint16_t, 36> indices_; //< インデックスデータ配列
	std::string textureName_; //< 使用テクスチャ名

	DxVertexBuffer<VertexData> vertexBuffer_; //< 頂点バッファ
	DxIndexBuffer<uint16_t> indexBuffer_; //< インデックスバッファ
	WorldTransform worldTransform_; //< ワールド変換情報
};