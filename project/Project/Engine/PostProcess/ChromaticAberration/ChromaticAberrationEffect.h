#pragma once
/* ========================================================================
/* include space
/* ===================================================================== */
#include <Engine/PostProcess/Interface/IPostEffectPass.h>
#include <Engine/Graphics/Pipeline/Pso/PsoDetails.h>
#include <Engine/Graphics/Buffer/DxConstantBuffer.h>

#include <externals/imgui/imgui.h>

struct ChromaticAberrationSettings{
	float intensity = 0.0f; // 0で効果なし
	float _pad[3] = {};   // 16byteアラインのためのパディング（安全）
};

class ChromaticAberrationEffect : public IPostEffectPass{
public:
	// ---- IPostEffectPass ----
	void Initialize(const PipelineSet& psoSet);
	void Apply(ID3D12GraphicsCommandList* cmd,
			   D3D12_GPU_DESCRIPTOR_HANDLE inputSRV,
			   IRenderTarget* outputRT) override;

	const std::string GetName() const override{ return "ChromaticAberration"; }

	// UI 
	void ShowImGui() override;
	void ResetParameters() override;
	void Tick(float /*dt*/) override{}

	float GetIntensity() const{ return intensity_; }
	void  SetIntensity(float intensity);

private:
	PipelineSet psoSet_;
	DxConstantBuffer<ChromaticAberrationSettings> settingsBuffer_;

	float intensity_ = 0.0f; // 既定はOFF（Tweenのauto-disableと相性が良い）

	// 操作しやすい想定レンジ（必要に応じて調整）
	static constexpr float kMin = 0.0f;
	static constexpr float kMax = 2.0f;
};
