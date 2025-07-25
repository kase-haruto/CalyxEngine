#pragma once

/* ========================================================================
/* include space
/* ===================================================================== */
#include <Engine/Foundation/Math/Vector2.h>
#include <Engine/Graphics/Buffer/DxConstantBuffer.h>
#include <Engine/Graphics/Pipeline/Pso/PsoDetails.h>
#include <Engine/PostProcess/Interface/IPostEffectPass.h>

class Vignette :
	public IPostEffectPass {
private:
	//===================================================================*/
	//		public methods
	//===================================================================*/

	struct VignetteParameter {
		float strength = 0.8f;  // 暗くする強さ (0.0 ～ 1.0)
		float radius = 0.85f;   // 暗くなり始める位置 (0.0 ～ 1.0)
	};
public:
	void Initialize(const PipelineSet& psoSet);
	void Apply(ID3D12GraphicsCommandList* cmd,
			   D3D12_GPU_DESCRIPTOR_HANDLE inputSRV,
			   IRenderTarget* outputRT) override;

	void ShowImGui()override;
	const std::string GetName() const override { return "Vignette"; }


	//===================================================================*/
	//		private methods
	//===================================================================*/
private:

	// pso ================================================================*/
	PipelineSet psoSet_;
	VignetteParameter param_;
	DxConstantBuffer<VignetteParameter> buffer_;
};




