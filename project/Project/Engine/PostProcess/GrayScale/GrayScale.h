#pragma once

#include <Engine/Graphics/Pipeline/Pso/PsoDetails.h>
#include <Engine/PostProcess/FullscreenDrawer.h>
#include <Engine/PostProcess/Interface/IPostEffectPass.h>

class GrayScaleEffect
	: public IPostEffectPass{
public:
	void Initialize(const PipelineSet& psoSet);
	void Apply(ID3D12GraphicsCommandList* cmd,
			   D3D12_GPU_DESCRIPTOR_HANDLE inputSRV,
			   IRenderTarget* outputRT) override;

	const std::string GetName() const override{ return "GrayScale"; }

private:
	PipelineSet psoSet_;
};


