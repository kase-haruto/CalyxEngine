#include "SwapChainRenderTarget.h"
#include <Engine/Application/System/Enviroment.h>
#include <Engine/Graphics/Context/GraphicsGroup.h>
#include <Engine/Graphics/Descriptor/SrvLocator.h>
#include <cassert>

void SwapChainRenderTarget::Initialize(DxSwapChain* swapChain,
									   ID3D12DescriptorHeap* rtvHeap,
									   UINT rtvDescriptorSize) {
	swapChain_ = swapChain;
	rtvHeap_ = rtvHeap;
	rtvDescriptorSize_ = rtvDescriptorSize;

	UINT backBufferCount = 2;
	currentStates_.resize(backBufferCount, D3D12_RESOURCE_STATE_PRESENT);

	viewport_ = { 0.0f, 0.0f, kWindowWidth, kWindowHeight, 0.0f, 1.0f };
	scissorRect_ = { 0, 0, kWindowWidth, kWindowHeight };

	// SRV を作成（ここでは BackBuffer[0] 用だが本来は毎フレーム更新が理想）
	auto device = GraphicsGroup::GetInstance()->GetDevice();
	auto resource = swapChain_->GetBackBuffer(0);
	if (resource) {
		auto [cpu, gpu] = SrvLocator::AllocateSrv();
		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
		srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM; // BackBuffer のフォーマットに合わせる
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srvDesc.Texture2D.MipLevels = 1;

		device->CreateShaderResourceView(resource.Get(), &srvDesc, cpu);
		srvCpuHandle_ = cpu;
		srvGpuHandle_ = gpu;
		hasSrv_ = true;
	}
}




void SwapChainRenderTarget::SetBufferIndex(UINT index){
	bufferIndex_ = index;
}

D3D12_CPU_DESCRIPTOR_HANDLE SwapChainRenderTarget::GetRTV() const{
	D3D12_CPU_DESCRIPTOR_HANDLE handle = rtvHeap_->GetCPUDescriptorHandleForHeapStart();
	handle.ptr += rtvDescriptorSize_ * bufferIndex_;
	return handle;
}

void SwapChainRenderTarget::SetRenderTarget(ID3D12GraphicsCommandList* cmdList){

	TransitionTo(cmdList, D3D12_RESOURCE_STATE_RENDER_TARGET);

	// ビューポートとシザー設定
	cmdList->RSSetViewports(1, &viewport_);
	cmdList->RSSetScissorRects(1, &scissorRect_);


	D3D12_CPU_DESCRIPTOR_HANDLE rtv = GetRTV();
	cmdList->OMSetRenderTargets(1, &rtv, FALSE, nullptr);
}


void SwapChainRenderTarget::Clear(ID3D12GraphicsCommandList* cmdList){
	TransitionTo(cmdList, D3D12_RESOURCE_STATE_RENDER_TARGET);
	float clearColor[] = {1.02f, 0.02f, 0.02f, 1.0f};
	cmdList->ClearRenderTargetView(GetRTV(), clearColor, 0, nullptr);
}

void SwapChainRenderTarget::TransitionTo(ID3D12GraphicsCommandList* commandList, D3D12_RESOURCE_STATES newState){
	if (bufferIndex_ >= currentStates_.size()) return; // 安全ガード

	D3D12_RESOURCE_STATES& currentState = currentStates_[bufferIndex_];
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

D3D12_GPU_DESCRIPTOR_HANDLE SwapChainRenderTarget::GetSRV() const {
	if (!hasSrv_) {
		return { 0 };
	}
	return srvGpuHandle_;
}