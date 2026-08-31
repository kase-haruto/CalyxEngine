#pragma once

/* ========================================================================
/* include space
/* ===================================================================== */
#include <Engine/Foundation/Math/Vector3.h>
#include <Engine/Graphics/Buffer/DxConstantBuffer.h>
#include <Engine/Graphics/Pipeline/Pso/PsoDetails.h>
#include <Engine/PostProcess/Interface/IPostEffectPass.h>

/*-----------------------------------------------------------------------------------------
 * Vignette
 * - 画面周辺を指定色で暗くするポストエフェクトクラス
 * - Effect設定の保持、定数バッファ転送、Fullscreen描画を担当する
 * - PipelineとRenderTargetの生成および所有は担当しない
 *---------------------------------------------------------------------------------------*/
/**
 * @brief Vignetteの機能を提供するクラスです。
 */
class Vignette :
	public IPostEffectPass {
private:
	//===================================================================*/
	//		public methods
	//===================================================================*/

	/*-----------------------------------------------------------------------------------------
	 * VignetteParameter
	 * - Vignette Shaderへ転送する定数バッファ用データ構造
	 * - 強度、効果半径、周辺色とGPUアラインメント用領域を保持する
	 *---------------------------------------------------------------------------------------*/
	/**
	 * @brief VignetteParameterに関するデータを保持する構造体です。
	 */
	struct VignetteParameter {
		float strength = 0.8f;  // 暗くする強さ (0.0 ～ 1.0)
		float radius = 0.85f;   // 暗くなり始める位置 (0.0 ～ 1.0)
		float padding0 = 0.0f;
		float padding1 = 0.0f;
		CalyxEngine::Vector3 color = {0.0f, 0.0f, 0.0f};
		float padding2 = 0.0f;
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




