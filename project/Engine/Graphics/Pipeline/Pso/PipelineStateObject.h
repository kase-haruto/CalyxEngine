#pragma once
/* ========================================================================
/* include space
/* ===================================================================== */
#include <Engine/Graphics/Pipeline/Pso/PsoDetails.h>

// lib
#include <d3d12.h>
#include <dxcapi.h>
#include <wrl.h>
#include <memory>

class PipelineStateObject {
public:
	PipelineStateObject() = default;

	// ルートシグネチャ設定
	void SetRootSignature(ID3D12RootSignature* root);

	// PSO 初期化（Graphics / Compute 両対応）
	bool Initialize(const D3D12_GRAPHICS_PIPELINE_STATE_DESC& desc);
	bool Initialize(const D3D12_COMPUTE_PIPELINE_STATE_DESC& desc);

	// アクセサ
	const PipelineSet& GetPipelineSet() const {
		return { pipelineState_.Get(), rootSignature_.Get() };
	}

	const Microsoft::WRL::ComPtr<ID3D12PipelineState>& GetPipelineState()  const { return pipelineState_; }
	const Microsoft::WRL::ComPtr<ID3D12RootSignature>& GetRootSignature()  const { return rootSignature_; }

	// シェーダブロブ保持
	void SetShaderBlobs(
		Microsoft::WRL::ComPtr<IDxcBlob> vs,
		Microsoft::WRL::ComPtr<IDxcBlob> ps) {
		vsBlob_ = std::move(vs);
		psBlob_ = std::move(ps);
		csBlob_.Reset();
		isCompute_ = false;
	}

	void SetShaderBlobCS(Microsoft::WRL::ComPtr<IDxcBlob> cs) {
		csBlob_ = std::move(cs);
		vsBlob_.Reset();
		psBlob_.Reset();
		isCompute_ = true;
	}

	bool IsCompute() const { return isCompute_; }

	bool Reload(ID3D12Device* /*device*/) { return false; }

private:
	Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature_;
	Microsoft::WRL::ComPtr<ID3D12PipelineState> pipelineState_;

	// シェーダブロブ保持
	Microsoft::WRL::ComPtr<IDxcBlob> vsBlob_;
	Microsoft::WRL::ComPtr<IDxcBlob> psBlob_;
	Microsoft::WRL::ComPtr<IDxcBlob> csBlob_;

	bool isCompute_ = false;
};