#pragma once

#include <Engine/Foundation/Math/Vector3.h>
#include <Engine/Graphics/Buffer/DxConstantBuffer.h>
#include <Engine/Graphics/Pipeline/Pso/PsoDetails.h>
#include <Engine/PostProcess/Interface/IPostEffectPass.h>

/**
 * @brief BloomEffectの機能を提供するクラスです。
 */
class BloomEffect : public IPostEffectPass {
private:
	/**
	 * @brief BloomParameterに関するデータを保持する構造体です。
	 */
	struct BloomParameter {
		float intensity = 0.7f;
		float threshold = 0.8f;
		float softKnee = 0.5f;
		float radius = 1.0f;
		CalyxEngine::Vector3 tint = {1.0f, 1.0f, 1.0f};
		float padding = 0.0f;
	};

public:
	void Initialize(const PipelineSet& psoSet);
	void Apply(ID3D12GraphicsCommandList* cmd,
			   D3D12_GPU_DESCRIPTOR_HANDLE inputSRV,
			   IRenderTarget* outputRT) override;

	const std::string GetName() const override { return "Bloom"; }
	void ShowImGui() override;
	void ResetParameters() override;
	nlohmann::json SaveParameters() const override;
	void LoadParameters(const nlohmann::json& params) override;
	bool GetFloatParameter(const std::string& name, float& out) const override;
	bool SetFloatParameter(const std::string& name, float value) override;

private:
	void SetIntensity(float value);
	void SetThreshold(float value);
	void SetSoftKnee(float value);
	void SetRadius(float value);

private:
	PipelineSet psoSet_{};
	BloomParameter param_{};
	DxConstantBuffer<BloomParameter> buffer_;
};
