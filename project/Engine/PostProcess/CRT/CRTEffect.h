#pragma once

#include <Engine/Graphics/Buffer/DxConstantBuffer.h>
#include <Engine/Graphics/Pipeline/Pso/PsoDetails.h>
#include <Engine/PostProcess/Interface/IPostEffectPass.h>
#include <Engine/Foundation/Math/Vector2.h>

class CRTEffect : public IPostEffectPass {
private:
	struct CRTParameter {
		CalyxMath::Vector2 screenSize;
		float time;
		float padding; // align 16
	};

public:
	void Initialize(const PipelineSet& psoSet);
	void Apply(ID3D12GraphicsCommandList* cmd,
			   D3D12_GPU_DESCRIPTOR_HANDLE inputSRV,
			   IRenderTarget* outputRT) override;
	const std::string GetName() const override { return "CRTEffect"; }

	void SetScreenSize(const CalyxMath::Vector2& size) {
		param_.screenSize = size;
	}

	void SetTime(float t) {
		param_.time = t;
	}

private:
	CRTParameter param_{ {1280, 720}, 0.0f, 0.0f };
	DxConstantBuffer<CRTParameter> crtBuffer_;
	PipelineSet psoSet_;
};
