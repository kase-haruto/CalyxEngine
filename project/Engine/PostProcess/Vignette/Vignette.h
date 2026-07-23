#pragma once

/* ========================================================================
/* include space
/* ===================================================================== */
#include <Engine/Foundation/Math/Vector2.h>
#include <Engine/Foundation/Math/Vector3.h>
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
		float padding[2]{};     // HLSL cbuffer の16バイト境界に合わせる
		CalyxEngine::Vector3 color{0.0f, 0.0f, 0.0f};
		float colorPadding = 0.0f;
	};
public:
	void Initialize(const PipelineSet& psoSet);
	void Apply(ID3D12GraphicsCommandList* cmd,
			   D3D12_GPU_DESCRIPTOR_HANDLE inputSRV,
			   IRenderTarget* outputRT) override;

	void ShowImGui()override;
	void ResetParameters() override;
	nlohmann::json SaveParameters() const override;
	void LoadParameters(const nlohmann::json& params) override;
	bool GetFloatParameter(const std::string& name, float& out) const override;
	bool SetFloatParameter(const std::string& name, float value) override;
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




