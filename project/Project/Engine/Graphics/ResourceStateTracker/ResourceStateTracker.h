#pragma once
#include <unordered_map>
#include <d3d12.h>
#include <wrl.h>

class ResourceStateTracker{
public:
	void SetResourceState(ID3D12Resource* resource, D3D12_RESOURCE_STATES state);
	D3D12_RESOURCE_STATES GetResourceState(ID3D12Resource* resource) const;

private:
	mutable std::unordered_map<ID3D12Resource*, D3D12_RESOURCE_STATES> resourceStates_;
};


