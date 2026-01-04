#include "PsoLibrary.h"

/////////////////////////////////////////////////////////////////////////////////////////
//		pso取得/生成
/////////////////////////////////////////////////////////////////////////////////////////
ID3D12PipelineState* PsoLibrary::GetOrCreate(const GraphicsPipelineDesc& desc) {
	auto it = psoCache_.find(desc);
	if (it != psoCache_.end()) return it->second->GetPipelineState().Get();

	auto pipeline = factory_->Create(desc);
	ID3D12PipelineState* pso = pipeline->GetPipelineState().Get();
	psoCache_[desc] = std::move(pipeline);
	return pso;
}

/////////////////////////////////////////////////////////////////////////////////////////
//		RootSignature取得
/////////////////////////////////////////////////////////////////////////////////////////
ID3D12RootSignature* PsoLibrary::GetRoot(const GraphicsPipelineDesc& desc) {
	GetOrCreate(desc);
	return psoCache_[desc]->GetRootSignature().Get();
}
