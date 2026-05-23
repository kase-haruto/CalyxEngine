#pragma once

#include <Engine/Graphics/Buffer/DxConstantBuffer.h>
#include <Engine/Graphics/Pipeline/Pso/PsoDetails.h>
#include <Engine/PostProcess/Interface/IPostEffectPass.h>

class BlendEffect : public IPostEffectPass {
private:
	struct BlendParameter {
		float opacity = 1.0f;
		float mode = 0.0f;
		float padding[2] = {};
	};

public:
	void Initialize(const PipelineSet& psoSet);
	void Apply(ID3D12GraphicsCommandList* cmd,
			   D3D12_GPU_DESCRIPTOR_HANDLE inputSRV,
			   IRenderTarget* outputRT) override;
	void Apply(ID3D12GraphicsCommandList* cmd,
			   D3D12_GPU_DESCRIPTOR_HANDLE inputA,
			   D3D12_GPU_DESCRIPTOR_HANDLE inputB,
			   IRenderTarget* outputRT);

	const std::string GetName() const override { return "Blend"; }
	void ShowImGui() override;
	void ResetParameters() override;
	nlohmann::json SaveParameters() const override;
	void LoadParameters(const nlohmann::json& params) override;
	bool GetFloatParameter(const std::string& name, float& out) const override;
	bool SetFloatParameter(const std::string& name, float value) override;

private:
	PipelineSet psoSet_{};
	BlendParameter param_{};
	DxConstantBuffer<BlendParameter> buffer_;
};
