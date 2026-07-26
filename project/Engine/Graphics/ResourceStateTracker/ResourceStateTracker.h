#pragma once
#include <unordered_map>
#include <d3d12.h>
#include <wrl.h>

/*-----------------------------------------------------------------------------------------
 * ResourceStateTracker
 * - DirectX12 Resourceごとの現在StateをCPU側で追跡するクラス
 * - Resource Barrier生成時に必要となる遷移前Stateの記録と取得を担当する
 * - ID3D12Resourceの所有権およびBarrier発行は担当しない
 *---------------------------------------------------------------------------------------*/
/**
 * @brief ResourceStateTrackerの機能を提供するクラスです。
 */
class ResourceStateTracker{
public:
	void SetResourceState(ID3D12Resource* resource, D3D12_RESOURCE_STATES state);
	D3D12_RESOURCE_STATES GetResourceState(ID3D12Resource* resource) const;

private:
	mutable std::unordered_map<ID3D12Resource*, D3D12_RESOURCE_STATES> resourceStates_;
};


