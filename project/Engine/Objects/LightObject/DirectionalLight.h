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
#include <Engine/Objects/3D/Actor/BaseGameObject.h>
#include <Engine/Objects/ConfigurableObject/ConfigurableObject.h>

/* config */
#include <Data/Engine/Configs/Scene/Objects/LightObjects/DirectionalLightConfig.h>

/* c++ */
#include <d3d12.h>
#include <wrl.h>

struct DirectionalLightData {
	CalyxMath::Vector4 color;	  // ライトの色
	CalyxMath::Vector3 direction; // ライトの向き
	float			   intensity; // 輝度
};

namespace CalyxGraphics {
	class DxCore;
}

/* ========================================================================
/*		方向性ライト
/* ===================================================================== */
class DirectionalLight
	: public SceneObject,
	  public IConfigurable {
public:
	DirectionalLight(const std::string& name);
	DirectionalLight();
	~DirectionalLight();

	void Update(float dt) override;
	void ShowGui() override;
	void DrawDebug();
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

	// config ============================================================
	void ApplyConfig();
	void ExtractConfig();
	void ApplyConfigFromJson(const nlohmann::json& j) override;
	void ExtractConfigToJson(nlohmann::json& j) const override;

	std::string_view GetTypeName() const override { return "DirectionalLight"; }
	std::string		 GetObjectTypeName() const override { return name_; }
	const CalyxMath::Matrix4x4& GetLightVP() const { return lightViewProj_; }
private:
	DxConstantBuffer<DirectionalLightData> constantBuffer_;
	DirectionalLightData				   lightData_ = {}; // ライトデータ

	std::shared_ptr<BaseGameObject> UiObject_ = nullptr;
	CalyxMath::Matrix4x4 lightViewProj_;
	

private:
	ConfigurableObject<DirectionalLightConfig> config_;
};