#include "PipelineStateManager.h"


PipelineStateManager::PipelineStateManager(ComPtr<ID3D12Device> device, std::shared_ptr<ShaderManager> shaderManager)
	: device_(device), shaderManager_(shaderManager) {}

PipelineStateManager::~PipelineStateManager() {}

////////////////////////////////////////////////////////////////////////////////////////////////////
// パイプラインステートの生成
////////////////////////////////////////////////////////////////////////////////////////////////////
bool PipelineStateManager::CreatePipelineState(
	PipelineType pipelineType,
	const std::wstring& vsPath,
	const std::wstring& psPath,
	const D3D12_ROOT_SIGNATURE_DESC& rootSignatureDesc,
	const D3D12_GRAPHICS_PIPELINE_STATE_DESC& psoDesc,
	BlendMode blendMode
){
	PipelineKey key {pipelineType, blendMode};

	if (pipelineStates_.find(key) != pipelineStates_.end()){
		return true;
	}

	auto pipeline = std::make_unique<PipelineState>(device_, shaderManager_);
	if (!pipeline->Initialize(vsPath, psPath, rootSignatureDesc, psoDesc, blendMode)){
		return false;
	}

	pipelineStates_[key] = std::move(pipeline);
	return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// 管理しているリソースの解放
////////////////////////////////////////////////////////////////////////////////////////////////////
void PipelineStateManager::Finalize(){
	shaderManager_.reset();
	device_.Reset();
	for (auto& pipeline : pipelineStates_){
		pipeline.second.reset();
	}
}

const PipelineSet PipelineStateManager::GetPipelineSet(PipelineType pipelineType, BlendMode blendMode) const {
	auto it = pipelineStates_.find({pipelineType, blendMode});
	assert(it != pipelineStates_.end());
	return it->second->GetPipelineSet();
}
////////////////////////////////////////////////////////////////////////////////////////////////////
// PipelineStateの取得
////////////////////////////////////////////////////////////////////////////////////////////////////
const Microsoft::WRL::ComPtr<ID3D12PipelineState>& PipelineStateManager::GetPipelineState(
	PipelineType pipelineType,
	BlendMode blendMode
) const{
	// (pipelineType, blendMode) をキーに探す
	PipelineKey key {pipelineType, blendMode};
	auto it = pipelineStates_.find(key);
	assert(it != pipelineStates_.end());
	return it->second->GetPipelineState();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// RootSignatureの取得
////////////////////////////////////////////////////////////////////////////////////////////////////
const Microsoft::WRL::ComPtr<ID3D12RootSignature>& PipelineStateManager::GetRootSignature(
	PipelineType pipelineType,
	BlendMode blendMode
) const{
	PipelineKey key {pipelineType, blendMode};
	auto it = pipelineStates_.find(key);
	assert(it != pipelineStates_.end());
	return it->second->GetRootSignature();
}
