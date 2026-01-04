#pragma once
#include <Engine/PostProcess/Interface/IPostEffectPass.h>
#include <Engine/Graphics/Pipeline/Pso/PsoDetails.h>

class CopyImageEffect :
	public IPostEffectPass {
public:
	void Initialize(const PipelineSet& psoSet);
	void Apply(ID3D12GraphicsCommandList* cmd,
			   D3D12_GPU_DESCRIPTOR_HANDLE inputSRV,
			   IRenderTarget* outputRT) override;
	const std::string GetName() const override { return "CopyImage"; }

private:
	PipelineSet psoSet_;
};