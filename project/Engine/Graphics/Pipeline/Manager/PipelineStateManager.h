#pragma once
/* ========================================================================
/*	include space
/* ===================================================================== */
// engine
#include <Engine/Graphics/Pipeline/PipelineKey.h>
#include <Engine/Graphics/Pipeline/Pso/PipelineState.h>
#include <Engine/Graphics/Pipeline/Shader/ShaderManager.h>

/* c++ */
#include <d3d12.h>
#include <string>
#include <unordered_map>

/* ========================================================================
/*	psoマネージャ
/* ===================================================================== */
class PipelineStateManager {
private:
	template <class T>
	using ComPtr = Microsoft::WRL::ComPtr<T>;

public:
	PipelineStateManager(ComPtr<ID3D12Device> device, std::shared_ptr<ShaderManager> shaderManager)
		: device_(device), shaderManager_(shaderManager) {}
	~PipelineStateManager();

	bool CreatePipelineState(
		PipelineType							  pipelineType,
		const std::wstring&						  vsPath,
		const std::wstring&						  psPath,
		const D3D12_ROOT_SIGNATURE_DESC&		  rootSignatureDesc,
		const D3D12_GRAPHICS_PIPELINE_STATE_DESC& psoDesc,
		BlendMode								  blendMode);

	void Finalize();

	const PipelineSet GetPipelineSet(
		PipelineType pipelineType,
		BlendMode	 blendMode) const {
		auto it = pipelineStates_.find({pipelineType, blendMode});
		assert(it != pipelineStates_.end());
		return it->second->GetPipelineSet();
	}

	const ComPtr<ID3D12PipelineState>& GetPipelineState(
		PipelineType pipelineType,
		BlendMode	 blendMode) const;

	const ComPtr<ID3D12RootSignature>& GetRootSignature(
		PipelineType pipelineType,
		BlendMode	 blendMode) const;

private:
	ComPtr<ID3D12Device>											device_;
	std::shared_ptr<ShaderManager>									shaderManager_;
	std::unordered_map<PipelineKey, std::unique_ptr<PipelineState>> pipelineStates_;
};
