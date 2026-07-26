#include "SceneDepthResource.h"

#include "Engine/Graphics/Descriptor/DescriptorAllocator.h"


void CalyxEngine::SceneDepthResource::Initialize(ID3D12Device* device, uint32_t w, uint32_t h) {
	// Scene解像度と一致するDepth専用TextureをDefault Heapへ生成する。
	D3D12_RESOURCE_DESC desc{};
	desc.Dimension		  = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	desc.Width			  = w;
	desc.Height			  = h;
	desc.DepthOrArraySize = 1;
	desc.MipLevels		  = 1;
	desc.Format			  = DXGI_FORMAT_D32_FLOAT;
	desc.SampleDesc.Count = 1;
	desc.Flags			  = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

	// Optimized Clear Valueを作成時に指定し、毎フレームのDepth Clearを効率化する。
	D3D12_CLEAR_VALUE clear{};
	clear.Format			 = DXGI_FORMAT_D32_FLOAT;
	clear.DepthStencil.Depth = 1.0f;

	D3D12_HEAP_PROPERTIES heap{};
	heap.Type = D3D12_HEAP_TYPE_DEFAULT;

	// 初回PassからDepth書込み可能な状態で作成し、不要な初期Barrierを避ける。
	device->CreateCommittedResource(
		&heap,
		D3D12_HEAP_FLAG_NONE,
		&desc,
		D3D12_RESOURCE_STATE_DEPTH_WRITE,
		&clear,
		IID_PPV_ARGS(resource_.ReleaseAndGetAddressOf()));

	// Descriptor HeapのLifetimeはAllocatorが管理し、このResourceは割当Handleだけを保持する。
	auto handle = DescriptorAllocator::Allocate(DescriptorUsage::Dsv);
	dsv_		= handle.cpu;
	device->CreateDepthStencilView(resource_.Get(), nullptr, dsv_);
}

void CalyxEngine::SceneDepthResource::Transition(
	ID3D12GraphicsCommandList* cmd,
	D3D12_RESOURCE_STATES newState)
{
	// 同一状態へのBarrierは発行せず、CommandListと追跡状態を常に対で更新する。
	if (currentState_ == newState) return;

	D3D12_RESOURCE_BARRIER barrier{};
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier.Transition.pResource   = resource_.Get();
	barrier.Transition.StateBefore = currentState_;
	barrier.Transition.StateAfter  = newState;
	barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

	// 全Subresourceを一括遷移し、Depth Texture全体の利用状態を統一する。
	cmd->ResourceBarrier(1, &barrier);
	currentState_ = newState;
}

void CalyxEngine::SceneDepthResource::BindForWrite(ID3D12GraphicsCommandList* cmd)
{
	Transition(cmd, D3D12_RESOURCE_STATE_DEPTH_WRITE);
}
