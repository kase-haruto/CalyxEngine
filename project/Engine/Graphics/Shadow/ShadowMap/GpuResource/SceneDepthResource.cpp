#include "SceneDepthResource.h"

#include "Engine/Graphics/Descriptor/DescriptorAllocator.h"


void CalyxGraphics::SceneDepthResource::Initialize(ID3D12Device* device, uint32_t w, uint32_t h) {
	D3D12_RESOURCE_DESC desc{};
	desc.Dimension		  = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	desc.Width			  = w;
	desc.Height			  = h;
	desc.DepthOrArraySize = 1;
	desc.MipLevels		  = 1;
	desc.Format			  = DXGI_FORMAT_D32_FLOAT;
	desc.SampleDesc.Count = 1;
	desc.Flags			  = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

	D3D12_CLEAR_VALUE clear{};
	clear.Format			 = DXGI_FORMAT_D32_FLOAT;
	clear.DepthStencil.Depth = 1.0f;

	D3D12_HEAP_PROPERTIES heap{};
	heap.Type = D3D12_HEAP_TYPE_DEFAULT;

	device->CreateCommittedResource(
		&heap,
		D3D12_HEAP_FLAG_NONE,
		&desc,
		D3D12_RESOURCE_STATE_DEPTH_WRITE,
		&clear,
		IID_PPV_ARGS(resource_.ReleaseAndGetAddressOf()));

	auto handle = DescriptorAllocator::Allocate(DescriptorUsage::Dsv);
	dsv_		= handle.cpu;
	device->CreateDepthStencilView(resource_.Get(), nullptr, dsv_);
}

void CalyxGraphics::SceneDepthResource::Transition(
	ID3D12GraphicsCommandList* cmd,
	D3D12_RESOURCE_STATES newState)
{
	if (currentState_ == newState) return;

	D3D12_RESOURCE_BARRIER barrier{};
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier.Transition.pResource   = resource_.Get();
	barrier.Transition.StateBefore = currentState_;
	barrier.Transition.StateAfter  = newState;
	barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

	cmd->ResourceBarrier(1, &barrier);
	currentState_ = newState;
}

void CalyxGraphics::SceneDepthResource::BindForWrite(ID3D12GraphicsCommandList* cmd)
{
	Transition(cmd, D3D12_RESOURCE_STATE_DEPTH_WRITE);
}