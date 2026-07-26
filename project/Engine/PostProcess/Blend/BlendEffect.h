#pragma once

#include <Engine/Graphics/Buffer/DxConstantBuffer.h>
#include <Engine/Graphics/Pipeline/Pso/PsoDetails.h>
#include <Engine/PostProcess/Interface/IPostEffectPass.h>

/*-----------------------------------------------------------------------------------------
 * BlendEffect
 * - 1枚または2枚の描画結果を合成するポストエフェクトクラス
 * - 合成パラメータ保持と描画命令登録を担当
 * - 入出力RenderTargetと実行順序の所有・管理は行わない
 *---------------------------------------------------------------------------------------*/
class BlendEffect : public IPostEffectPass {
private:
	/*-----------------------------------------------------------------------------------------
	 * BlendParameter
	 * - ブレンドシェーダーへ転送する定数バッファ用データ構造
	 * - GPU側の16バイト配置と一致する合成設定を保持
	 *---------------------------------------------------------------------------------------*/
	struct BlendParameter {
		float opacity = 1.0f; //< 入力Bの合成不透明度（0.0～1.0）
		float mode = 0.0f; //< シェーダーで選択するブレンドモード番号
		float padding[2] = {}; //< 定数バッファ16バイト境界調整用
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
	PipelineSet psoSet_{}; //< 所有権を持たないPSOとルートシグネチャの組
	BlendParameter param_{}; //< EditorとRuntimeで共有する現在の合成設定
	DxConstantBuffer<BlendParameter> buffer_; //< 合成設定をGPUへ転送する定数バッファ
};
