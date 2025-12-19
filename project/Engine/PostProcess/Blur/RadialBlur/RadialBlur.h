#pragma once
/* ========================================================================
/* include space
/* ===================================================================== */
#include <Engine/Foundation/Math/Vector2.h>
#include <Engine/Graphics/Buffer/DxConstantBuffer.h>
#include <Engine/Graphics/Pipeline/Pso/PsoDetails.h>
#include <Engine/PostProcess/Interface/IPostEffectPass.h>

#include <externals/imgui/imgui.h>

class RadialBlurEffect
	: public IPostEffectPass {
private:
	struct BlurParameter {
		CalyxMath::Vector2 center; // [0,1] normalized
		float   width;  // 強度（シェーダー側の扱いに合わせて "width" 名）
		float   _pad;   // 16byte アラインメント用（必要なら）
	};

public:
	// ---------------- IPostEffectPass ----------------
	void Initialize(const PipelineSet& psoSet);
	void Apply(ID3D12GraphicsCommandList* cmd,
			   D3D12_GPU_DESCRIPTOR_HANDLE inputSRV,
			   IRenderTarget* outputRT) override;

	const std::string GetName() const override { return "RadialBlur"; }

	void ShowImGui() override;
	void ResetParameters() override;
	void Tick(float /*dt*/) override {} // 今回は未使用

	// ---------------- Runtime API（Tween用にgetter/setter揃える） ----------------
	float   GetWidth() const           { return blurParam_.width; }
	void    SetWidth(float width);                     // 強度（0で効果無し）
	CalyxMath::Vector2 GetCenter() const          { return blurParam_.center; }
	void    SetCenter(const CalyxMath::Vector2& uv);

private:
	// パラメータ
	BlurParameter blurParam_{ {0.5f, 0.5f}, 0.0f, 0.0f }; // 既定はOFF（0）
	DxConstantBuffer<BlurParameter> blurBuffer_;

	// PSO
	PipelineSet psoSet_;

	// 安全なレンジ（必要に応じて変更）
	static constexpr float kMinWidth = 0.0f;
	static constexpr float kMaxWidth = 2.0f;
};