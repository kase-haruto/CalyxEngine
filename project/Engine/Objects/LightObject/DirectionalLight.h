#pragma once
/* ========================================================================
/* include space
/* ===================================================================== */

/* math */
#include <Engine/Foundation/Math/Vector3.h>
#include <Engine/Foundation/Math/Vector4.h>

/* engine */
#include <Engine/Foundation/Serialization/SerializableObject.h>
#include <Engine/Graphics/Buffer/DxConstantBuffer.h>
#include <Engine/Graphics/Pipeline/PipelineType.h>
#include <Engine/Objects/3D/Actor/BaseGameObject.h>
#include <Engine/Objects/ConfigurableObject/ConfigurableObject.h>

/* config */
#include <Data/Engine/Configs/Scene/Objects/LightObjects/DirectionalLightConfig.h>

/* c++ */
#include <d3d12.h>
#include <wrl.h>
#include <cstdint>

struct DirectionalLightData {
	CalyxMath::Vector4 color;	  //< ライトの色
	CalyxMath::Vector3 direction; //< ライトの向き
	float			   intensity; //< 輝度
};

// GPU転送用のPOD構造体（シェーダーのcbuffer RaytracingShadowParamConstantsと一致）
struct ShadowParamGpu {
	float shadowRayEps;
	float baseAngularRadius;
	float minShadow;
	uint32_t isSoft; // cbuffer bool(32bit)と整合
};

namespace CalyxGraphics {
	class DxCore;
}

/*-----------------------------------------------------------------------------------------
 * DirectionalLight
 * - 方向性ライトクラス
 * - 平行光源の色、向き、輝度の管理、およびシャドウマップ用の行列計算を担当
 *---------------------------------------------------------------------------------------*/
class DirectionalLight
	: public SceneObject,
	  public IConfigurable {
public:
	struct ShadowParam
		: public CalyxEngine::SerializableObject {
		ShadowParam();
		CalyxEngine::ParamPath GetParamPath() const override;

		float shadowRayEps		= 0.01f;
		float baseAngularRadius = 0.05f;
		float minShadow			= 0.1f;
		bool  isSoft			= false;
	};
	ShadowParam shadow_;
	int			shadowSampleCount = 8;
	float		pad[2];

public:
	//===================================================================*/
	//                   public methods
	//===================================================================*/
	/**
	 * \brief コンストラクタ
	 * \param name オブジェクト名
	 */
	DirectionalLight(const std::string& name);

	/**
	 * \brief コンストラクタ
	 */
	DirectionalLight();

	/**
	 * \brief デストラクタ
	 */
	~DirectionalLight();

	/**
	 * \brief 更新処理
	 * \param dt デルタタイム
	 */
	void Update(float dt) override;

	/**
	 * \brief ImGui表示
	 */
	void ShowGui() override;

	/**
	 * \brief デバッグ描画
	 */
	void DrawDebug();

	/**
	 * \brief 常時更新処理
	 * \param dt デルタタイム
	 */
	void AlwaysUpdate(float dt);

	/**
	 * \brief GPUにデータをアップロード
	 */
	void UploadToGpu();

	/**
	 * \brief ライトのビュー・プロジェクション行列を更新
	 * \param sceneBounds シーンのAABB
	 */
	void UpdateLightVP(const AABB& sceneBounds);

	/**
	 * \brief コマンドリストにセット
	 * \param commandList コマンドリスト
	 * \param type パイプラインタイプ
	 */
	void SetCommand(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList, PipelineType type);

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

	/**
	 * \brief タイプ名を取得
	 * \return タイプ名
	 */
	std::string_view GetTypeName() const override { return "DirectionalLight"; }

	/**
	 * \brief オブジェクトタイプ名を取得
	 * \return オブジェクトタイプ名
	 */
	std::string GetObjectTypeName() const override { return name_; }

	/**
	 * \brief ライトのビュープロジェクション行列を取得
	 * \return 行列
	 */
	const CalyxMath::Matrix4x4& GetLightVP() const { return lightViewProj_; }

private:
	//===================================================================*/
	//                    private member variables
	//===================================================================*/
	DxConstantBuffer<DirectionalLightData> constantBuffer_; //< 定数バッファ
	DirectionalLightData				   lightData_ = {}; //< ライトデータ本体

	std::shared_ptr<BaseGameObject> UiObject_ = nullptr; //< UI用オブジェクト
	CalyxMath::Matrix4x4			lightViewProj_;		 //< ライト用ビュープロジェクション行列

	ConfigurableObject<DirectionalLightConfig> config_; //< コンフィグ管理

	DxConstantBuffer<ShadowParamGpu> shadowParamCB_;
};