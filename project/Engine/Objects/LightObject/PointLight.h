#pragma once
/* ========================================================================
/* include space
/* ===================================================================== */
/* math */
#include <Engine/Foundation/Math/Vector3.h>
#include <Engine/Foundation/Math/Vector4.h>

/* engine */
#include <Engine/Graphics/Buffer/DxConstantBuffer.h>
#include <Engine/Graphics/Pipeline/PipelineType.h>
#include <Engine/Objects/ConfigurableObject/ConfigurableObject.h>

/* config */
#include <Data/Engine/Configs/Scene/Objects/LightObjects/PointLightConfig.h>

/* lib */
#include <d3d12.h>
#include <wrl.h>

struct PointLightData{
	CalyxEngine::Vector4 color; //< ライトの色
	CalyxEngine::Vector3 position; //< ライトの位置
	float intensity; //< 光度
	float radius; //< ライトの届く最大距離
	float decay; //< 減衰率
	float pad[2]; //< パディング
};

namespace CalyxEngine {
	class DxCore;
}

/*-----------------------------------------------------------------------------------------
 * PointLight
 * - ポイントライト（点光源）クラス
 * - 指定座標を中心に全方位へ光を放つ光源のパラメータ管理を担当
 *---------------------------------------------------------------------------------------*/
class PointLight
	: public SceneObject,
	public IConfigurable{
public:
	//===================================================================*/
	//                   public methods
	//===================================================================*/
	/**
	 * \brief コンストラクタ
	 * \param name オブジェクト名
	 */
	PointLight(const std::string& name);

	/**
	 * \brief コンストラクタ
	 */
	PointLight();

	/**
	 * \brief デストラクタ
	 */
	~PointLight();

	/**
	 * \brief 初期化
	 */
	void Initialize();

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
	 * \brief ImGui表示
	 */
	void ShowGui()override;

	/**
	 * \brief GPUにデータをアップロード
	 */
	void UploadToGpu();

	/**
	 * \brief コマンドリストにセット
	 * \param commandList コマンドリスト
	 * \param type パイプラインタイプ
	 */
	void SetCommand(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList, PipelineType type);

	/**
	 * \brief タイプ名を取得
	 * \return タイプ名
	 */
	std::string_view GetObjectClassName() const override{ return "PointLight"; }

	/**
	 * \brief オブジェクトタイプ名を取得
	 * \return オブジェクトタイプ名
	 */
	std::string GetObjectTypeName()const override{ return name_; }

	//===================================================================*/
	//                   config
	//===================================================================*/
	/**
	 * \brief コンフィグを適用
	 */
	void ApplyConfig();

	/**
	 * \brief コンフィグを抽出
	 */
	void ExtractConfig();

	/**
	 * \brief JSONからコンフィグを適用
	 * \param j JSON
	 */
	void ApplyConfigFromJson(const nlohmann::json& j) override;

	/**
	 * \brief コンフィグをJSONに抽出
	 * \param j JSON
	 */
	void ExtractConfigToJson(nlohmann::json& j) const override;

private:
	//===================================================================*/
	//                    private member variables
	//===================================================================*/
	DxConstantBuffer<PointLightData> constantBuffer_; //< 定数バッファ
	PointLightData lightData_ = {}; //< ライトデータ本体

	ConfigurableObject<PointLightConfig> config_; //< コンフィグ管理
};