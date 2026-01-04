#include "RenderTargetCollection.h"
#include <d3d12.h>

void RenderTargetCollection::Add(const std::string& name, std::unique_ptr<IRenderTarget> target){
	targets_[name] = std::move(target);
}

IRenderTarget* RenderTargetCollection::Get(const std::string& name) const{
	auto it = targets_.find(name);
	return (it != targets_.end()) ? it->second.get() : nullptr;
}

void RenderTargetCollection::ClearAll(ID3D12GraphicsCommandList* cmdList){
	for (auto& [_, target] : targets_){
		target->Clear(cmdList);
	}
}
