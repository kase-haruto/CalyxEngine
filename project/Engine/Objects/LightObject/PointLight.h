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
	Vector4 color;		//ライトの色
	Vector3 position;	//ライトの位置
	float intensity;	//光度
	float radius;		//ライトの届く最大距離
	float decay;		//減衰率
	float pad[2];
};

class DxCore;

class PointLight
	: public SceneObject,
	public IConfigurable{
public:
	PointLight(const std::string& name);
	PointLight();
	~PointLight();

	void Initialize();
	void Update()override;
	void ShowGui()override;
	void UploadToGpu();
	void SetCommand(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList, PipelineType type);
	std::string_view GetTypeName() const override{ return "PointLight"; }
	std::string GetObjectTypeName()const override{ return name_; }

	// config ============================================================
	void ApplyConfig();
	void ExtractConfig();
	void ApplyConfigFromJson(const nlohmann::json& j) override;
	void ExtractConfigToJson(nlohmann::json& j) const override;


private:
	DxConstantBuffer<PointLightData> constantBuffer_;
	PointLightData lightData_ = {}; // ライトデータ

private:
	ConfigurableObject<PointLightConfig> config_;
};

