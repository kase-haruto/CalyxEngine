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
	Vector4 color;	   // ライトの色
	Vector3 direction; // ライトの向き
	float	intensity; // 輝度
};

class DxCore;

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

	/// <summary>
	/// gpuに転送
	/// </summary>
	void UploadToGpu();

	/// <summary>
	/// コマンドを積む
	/// </summary>
	/// <param name="commandList"></param>
	/// <param name="type"></param>
	void SetCommand(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList, PipelineType type);


	// config ============================================================
	void ApplyConfig();
	void ExtractConfig();
	void ApplyConfigFromJson(const nlohmann::json& j) override;
	void ExtractConfigToJson(nlohmann::json& j) const override;

	std::string_view GetTypeName() const override { return "DirectionalLight"; }
	std::string GetObjectTypeName() const override { return name_; }

private:
	DxConstantBuffer<DirectionalLightData> constantBuffer_;
	DirectionalLightData				   lightData_ = {}; // ライトデータ

	std::shared_ptr<BaseGameObject> UiObject_ = nullptr;

private:
	ConfigurableObject<DirectionalLightConfig> config_;
};