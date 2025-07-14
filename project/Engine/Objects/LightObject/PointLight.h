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
	: public SceneObject
	, public ConfigurableObject<PointLightConfig>{
public:
	PointLight(const std::string& name);
	PointLight() = default;
	~PointLight();

	void Initialize();
	void Update()override;
	void ShowGui()override;
	void UploadToGpu();
	void SetCommand(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList, PipelineType type);
	std::string_view GetTypeName() const override{ return "PointLight"; }
	// config ============================================================
	void ApplyConfig()override;
	void ExtractConfig()override;

	std::string GetObjectTypeName()const override { return name_; }

private:
	DxConstantBuffer<PointLightData> constantBuffer_;
	PointLightData lightData_ = {}; // ライトデータ
};

