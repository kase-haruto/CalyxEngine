#include "ResourceStateTracker.h"


void ResourceStateTracker::SetResourceState(ID3D12Resource* resource, D3D12_RESOURCE_STATES state){
	if (resource == nullptr) return;
	resourceStates_[resource] = state;
}

D3D12_RESOURCE_STATES ResourceStateTracker::GetResourceState(ID3D12Resource* resource) const{
	if (resource == nullptr) return D3D12_RESOURCE_STATE_COMMON;

	auto it = resourceStates_.find(resource);
	if (it != resourceStates_.end()){
		return it->second;
	}
	return D3D12_RESOURCE_STATE_COMMON; // デフォルト（未初期化時の仮状態）
}
