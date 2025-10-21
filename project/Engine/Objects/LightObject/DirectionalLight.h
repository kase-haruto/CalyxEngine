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
#include <Engine/Graphics/Buffer/DxConstantBuffer.h>
#include <Engine/Objects/3D/Actor/BaseGameObject.h>

/* config */
#include <Data/Engine/Configs/Scene/Objects/LightObjects/DirectionalLightConfig.h>

/* c++ */
#include<d3d12.h>
#include<wrl.h>

struct DirectionalLightData {
	Vector4 color;     //ライトの色
	Vector3 direction; //ライトの向き
	float   intensity; //輝度
};

class DxCore;

class DirectionalLight
	: public SceneObject,
	  public IConfigurable {
public:
	DirectionalLight(const std::string& name);
	DirectionalLight();
	~DirectionalLight();


	void Update(float dt) override;
	void DrawDebug();

	void AlwaysUpdate(float dt);
	void UploadToGpu();
	void SetCommand(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList,PipelineType type);
	void ShowGui() override;

	std::string_view GetTypeName() const override { return "DirectionalLight"; }

	// config ============================================================
	void ApplyConfig();
	void ExtractConfig();
	void ApplyConfigFromJson(const nlohmann::json& j) override;
	void ExtractConfigToJson(nlohmann::json& j) const override;

	std::string GetObjectTypeName() const override { return name_; }

private:
	DxConstantBuffer<DirectionalLightData> constantBuffer_;
	DirectionalLightData                   lightData_ = {}; // ライトデータ

	std::shared_ptr<BaseGameObject> UiObject_ = nullptr;

private:
	ConfigurableObject<DirectionalLightConfig> config_;
};