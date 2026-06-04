#pragma once
/* ========================================================================
   include space
   ===================================================================== */
#include <Engine/objects/LightObject/DirectionalLight.h>
#include <Engine/objects/LightObject/PointLight.h>
#include <Engine/Graphics/Buffer/DxConstantBuffer.h>

#include <array>
#include <cstdint>
#include <memory>
#include <vector>

struct PointLightConstants {
	uint32_t count = 0;
	uint32_t pointLightShadowsEnabled = 1;
	uint32_t maxPointShadowLights = 2;
	float pointShadowContributionThreshold = 0.01f;
	std::array<PointLightData, 128> lights = {};
};

/*-----------------------------------------------------------------------------------------
 * LightLibrary
 * - ライト管理クラス
 * - シーン内のディレクショナルライト・ポイントライトの登録とGPUへのコマンド送信を管理
 *---------------------------------------------------------------------------------------*/
class LightLibrary {
public:
	LightLibrary()  = default;
	~LightLibrary() = default;

	/* GPU 同期 ---------------------------------------------------------*/
	void CyncGpu();
	void Clear();

	/* 登録／取得 --------------------------------------------------------*/
	void SetDirectionalLight(const std::shared_ptr<DirectionalLight>& light);
	void SetPointLight(const std::shared_ptr<PointLight>& light);
	void AddPointLight(const std::shared_ptr<PointLight>& light);
	void RemovePointLight(const std::shared_ptr<PointLight>& light);

	DirectionalLight* GetDirectionalLight() const { return directionalLight_.get(); }
	PointLight*       GetPointLight() const;
	const std::vector<std::weak_ptr<PointLight>>& GetPointLights() const { return pointLights_; }
	static constexpr uint32_t GetMaxPointLightCount() { return kMaxPointLightCount; }

	/* 描画コマンド ------------------------------------------------------*/
	void SetCommand(ID3D12GraphicsCommandList* cmdList,PipelineType pipelineType);
	void SetCommand(ID3D12GraphicsCommandList* cmdList,
	                PipelineType               pipelineType,
	                LightType                  lightType);

private:
	void EnsurePointLightBuffer();
	void CleanupPointLights();
	bool ContainsPointLight(const PointLight* light) const;

	static constexpr uint32_t kMaxPointLightCount = 16;

	std::shared_ptr<DirectionalLight> directionalLight_;
	std::vector<std::weak_ptr<PointLight>> pointLights_;
	DxConstantBuffer<PointLightConstants> pointLightBuffer_;
	PointLightConstants pointLightConstants_ = {};
};
