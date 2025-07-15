#pragma once
/* ========================================================================
/* include space
/* ===================================================================== */

/* math */
#include <Engine/Foundation/Math/Vector3.h>
#include <Engine/Foundation/Math/Vector4.h>

/* engine */
#include <Engine/Objects/ConfigurableObject/ConfigurableObject.h>
#include <Engine/Graphics/Pipeline/PipelineType.h>
#include <Engine/Lighting/LightData.h>
#include <Engine/Graphics/Buffer/DxConstantBuffer.h>

/* config */
#include <Data/Engine/Configs/Scene/Objects/LightObjects/DirectionalLightConfig.h>

/* c++ */
#include<d3d12.h>
#include<wrl.h>

struct DirectionalLightData{
	Vector4 color;		//ライトの色
	Vector3 direction;	//ライトの向き
	float intensity;	//輝度
};

class DxCore;

class DirectionalLight
	: public SceneObject
	, public ConfigurableObject<DirectionalLightConfig>{
public:
	DirectionalLight(const std::string& name);
	DirectionalLight() = default;
	~DirectionalLight();

	void Initialize();
	void Update()override;
	void UploadToGpu();
	void SetCommand(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList, PipelineType type);
	void ShowGui()override;
	std::string_view GetTypeName() const override{ return "DirectionalLight"; }
	// config ============================================================
	void ApplyConfig()override;
	void ExtractConfig()override;

	std::string GetObjectTypeName()const override { return name_; }

private:
	DxConstantBuffer<DirectionalLightData> constantBuffer_;
	DirectionalLightData lightData_ = {};	// ライトデータ
};

