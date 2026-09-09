#pragma once

#include <Engine/Graphics/Buffer/DxConstantBuffer.h>
#include <Engine/Graphics/Pipeline/Pso/PsoDetails.h>
#include <Engine/PostProcess/Interface/IPostEffectPass.h>

/** Adds animated monochrome film grain to the scene color. */
class FilmGrainEffect final : public IPostEffectPass {
public:
	struct FilmGrainParameter {
		float intensity = 0.08f;
		float grainSize = 1.5f;
		float time = 0.0f;
		float speed = 1.0f;
	};

	void Initialize(const PipelineSet& psoSet);
	void Apply(ID3D12GraphicsCommandList* cmd,
		D3D12_GPU_DESCRIPTOR_HANDLE inputSRV,
		IRenderTarget* outputRT) override;
	void Tick(float dt) override;
	void ShowImGui() override;
	void ResetParameters() override;
	nlohmann::json SaveParameters() const override;
	void LoadParameters(const nlohmann::json& params) override;
	bool GetFloatParameter(const std::string& name, float& out) const override;
	bool SetFloatParameter(const std::string& name, float value) override;
	const std::string GetName() const override { return "FilmGrain"; }

private:
	PipelineSet psoSet_;
	FilmGrainParameter param_{};
	DxConstantBuffer<FilmGrainParameter> buffer_;
};
