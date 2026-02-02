#include "SwapChainRenderTarget.h"
#include <Engine/Application/System/Environment.h>
#include <Engine/Graphics/Context/GraphicsGroup.h>

void SwapChainRenderTarget::Initialize(DxSwapChain* swapChain){
	swapChain_ = swapChain;

	const UINT backBufferCount = swapChain_->GetSwapChainDesc().BufferCount;
	currentStates_.resize(backBufferCount, D3D12_RESOURCE_STATE_PRESENT);
	rtvHandles_.resize(backBufferCount);
	srvHandles_.resize(backBufferCount);

	viewport_ = {0.0f, 0.0f, kWindowWidth, kWindowHeight, 0.0f, 1.0f};
	scissorRect_ = {0, 0, kWindowWidth, kWindowHeight};

	auto device = GraphicsGroup::GetInstance()->GetDevice();

	for (UINT i = 0; i < backBufferCount; ++i){
		auto resource = swapChain_->GetBackBuffer(i);
		if (!resource) continue;

		// RTV
		rtvHandles_[i] = DescriptorAllocator::Allocate(DescriptorUsage::Rtv);
		device->CreateRenderTargetView(resource.Get(), nullptr, rtvHandles_[i].cpu);

		// SRV
		srvHandles_[i] = DescriptorAllocator::Allocate(DescriptorUsage::CbvSrvUav);
		
		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
		srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srvDesc.Texture2D.MostDetailedMip = 0;
		srvDesc.Texture2D.MipLevels = 1;
		srvDesc.Texture2D.PlaneSlice = 0;
		srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;

		device->CreateShaderResourceView(resource.Get(), &srvDesc, srvHandles_[i].cpu);
	}
}

void SwapChainRenderTarget::SetBufferIndex(UINT index){
	bufferIndex_ = index;
}

D3D12_CPU_DESCRIPTOR_HANDLE SwapChainRenderTarget::GetRTV() const{
	if (bufferIndex_ >= rtvHandles_.size()) return {0};
	return rtvHandles_[bufferIndex_].cpu;
}

D3D12_GPU_DESCRIPTOR_HANDLE SwapChainRenderTarget::GetSRV() const{
	if (bufferIndex_ >= srvHandles_.size() || srvHandles_[bufferIndex_].gpu.ptr == 0){
		return {0};
	}
	return srvHandles_[bufferIndex_].gpu;
}

void SwapChainRenderTarget::SetRenderTarget(ID3D12GraphicsCommandList* cmdList){
	TransitionTo(cmdList, D3D12_RESOURCE_STATE_RENDER_TARGET);
	cmdList->RSSetViewports(1, &viewport_);
	cmdList->RSSetScissorRects(1, &scissorRect_);

	D3D12_CPU_DESCRIPTOR_HANDLE rtv = GetRTV();
	if(dsv_.cpu.ptr == 0) {
		cmdList->OMSetRenderTargets(1, &rtv, FALSE, nullptr);
	} else {
		cmdList->OMSetRenderTargets(1, &rtv, FALSE, &dsv_.cpu);
	}
}

void SwapChainRenderTarget::Clear(ID3D12GraphicsCommandList* cmdList){
	TransitionTo(cmdList, D3D12_RESOURCE_STATE_RENDER_TARGET);
	float clearColor[] = {0.02f, 0.02f, 0.02f, 1.0f};
	cmdList->ClearRenderTargetView(GetRTV(), clearColor, 0, nullptr);
}

void SwapChainRenderTarget::TransitionTo(ID3D12GraphicsCommandList* commandList, D3D12_RESOURCE_STATES newState){
	if (bufferIndex_ >= currentStates_.size()) return;

	auto& currentState = currentStates_[bufferIndex_];
	if (currentState == newState) return;

	D3D12_RESOURCE_BARRIER barrier = {};
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	barrier.Transition.pResource = swapChain_->GetBackBuffer(bufferIndex_).Get();
	barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	barrier.Transition.StateBefore = currentState;
	barrier.Transition.StateAfter = newState;

	commandList->ResourceBarrier(1, &barrier);
	currentState = newState;
}

void SwapChainRenderTarget::ReleaseSRVs(){
	for (auto& handle : srvHandles_){
		if (handle.cpu.ptr != 0){
			DescriptorAllocator::Free(DescriptorUsage::CbvSrvUav, handle);
			handle = {};
		}
	}
	srvHandles_.clear();
}

void SwapChainRenderTarget::Resize(uint32_t width, uint32_t height) {
	viewport_ = D3D12_VIEWPORT{0.0f, 0.0f, static_cast<float>(width), static_cast<float>(height), 0.0f, 1.0f};
	scissorRect_ = D3D12_RECT{0, 0, static_cast<LONG>(width), static_cast<LONG>(height)};

	auto device = GraphicsGroup::GetInstance()->GetDevice();

	for (UINT i = 0; i < srvHandles_.size(); ++i) {
		auto resource = swapChain_->GetBackBuffer(i);
		if (!resource) continue;

		// RTV 再生成 (ハンドルは保持したまま)
		device->CreateRenderTargetView(resource.Get(), nullptr, rtvHandles_[i].cpu);

		// SRV 再生成
		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
		srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srvDesc.Texture2D.MostDetailedMip = 0;
		srvDesc.Texture2D.MipLevels = 1;
		srvDesc.Texture2D.PlaneSlice = 0;
		srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;

		device->CreateShaderResourceView(resource.Get(), &srvDesc, srvHandles_[i].cpu);
	}
}
