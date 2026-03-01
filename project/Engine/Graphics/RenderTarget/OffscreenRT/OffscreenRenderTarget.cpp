#include "OffscreenRenderTarget.h"
#include <Engine/Application/System/Environment.h>
#include <Engine/Graphics/Context/GraphicsGroup.h>

#include <cassert>
#include <d3dx12.h>
#include <stdexcept>

void OffscreenRenderTarget::Initialize(ID3D12Device* device, uint32_t width, uint32_t height, DXGI_FORMAT format, DescriptorHandle rtvHandle, DescriptorHandle dsvHandle) {
	rtvHandle_ = rtvHandle;
	dsvHandle_ = dsvHandle;

	// リソースの初期化
	viewport_	 = {0.0f, 0.0f, static_cast<float>(width), static_cast<float>(height), 0.0f, 1.0f};
	scissorRect_ = {0, 0, static_cast<LONG>(width), static_cast<LONG>(height)};
	// リソースの初期化
	if(!resource_) {
		resource_ = std::make_unique<DxGpuResource>();
		resource_->InitializeAsRenderTarget(device, width, height, format);
		resource_->CreateRTV(device, rtvHandle_.cpu);
		resource_->CreateSRV(device);
	} else {
		resource_->InitializeAsRenderTarget(device, width, height, format);
		resource_->CreateRTV(device, rtvHandle_.cpu);
		resource_->UpdateSRV(device);
	}

	// 初期状態を保存（RENDER_TARGET）
	resource_->SetCurrentState(D3D12_RESOURCE_STATE_RENDER_TARGET);

	// Depthバッファ作成
	if(!depthResource_) {
		depthResource_ = std::make_unique<DxGpuResource>();
		depthResource_->InitializeAsDepthStencil(device, width, height, DXGI_FORMAT_R32_TYPELESS);
		depthResource_->CreateDSV(device, dsvHandle_.cpu);
		depthResource_->CreateSRV(device);
	} else {
		depthResource_->InitializeAsDepthStencil(device, width, height, DXGI_FORMAT_R32_TYPELESS);
		depthResource_->CreateDSV(device, dsvHandle_.cpu);
		depthResource_->UpdateSRV(device);
	}
}

DxGpuResource* OffscreenRenderTarget::GetResource() const {
	return resource_.get();
}

D3D12_CPU_DESCRIPTOR_HANDLE OffscreenRenderTarget::GetRTV() const {
	return rtvHandle_.cpu;
}

D3D12_CPU_DESCRIPTOR_HANDLE OffscreenRenderTarget::GetDSV() const {
	return dsvHandle_.cpu;
}

D3D12_GPU_DESCRIPTOR_HANDLE OffscreenRenderTarget::GetSRV() const {
	return resource_->GetSRVGpuHandle();
}

D3D12_GPU_DESCRIPTOR_HANDLE OffscreenRenderTarget::GetDepthSRV() const {
	return depthResource_->GetSRVGpuHandle();
}

void OffscreenRenderTarget::SetDepthDSV(DescriptorHandle dsv) {
	dsvHandle_ = dsv;
}

D3D12_VIEWPORT OffscreenRenderTarget::GetViewport() const {
	return viewport_;
}

D3D12_RECT OffscreenRenderTarget::GetScissorRect() const {
	return scissorRect_;
}

void OffscreenRenderTarget::Clear(ID3D12GraphicsCommandList* commandList) {
	resource_->Transition(commandList, D3D12_RESOURCE_STATE_RENDER_TARGET);
	float clearColor[] = {0.1f, 0.1f, 0.1f, 1.0f};
	commandList->ClearRenderTargetView(rtvHandle_.cpu, clearColor, 0, nullptr);

	depthResource_->Transition(commandList, D3D12_RESOURCE_STATE_DEPTH_WRITE);
	commandList->ClearDepthStencilView(dsvHandle_.cpu, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
}

void OffscreenRenderTarget::SetRenderTarget(ID3D12GraphicsCommandList* commandList) {
	commandList->RSSetViewports(1, &viewport_);
	commandList->RSSetScissorRects(1, &scissorRect_);

	// OMSetRenderTargets
	commandList->OMSetRenderTargets(1, &rtvHandle_.cpu, FALSE, &dsvHandle_.cpu);
}

void OffscreenRenderTarget::TransitionTo(ID3D12GraphicsCommandList* cmdList, D3D12_RESOURCE_STATES newState) {
	resource_->Transition(cmdList, newState);
}

void OffscreenRenderTarget::Resize(uint32_t width, uint32_t height) {
	auto		device = GraphicsGroup::GetInstance()->GetDevice();
	DXGI_FORMAT format = resource_->Get()->GetDesc().Format;

	// 同一サイズなら何もしない
	auto desc = resource_->Get()->GetDesc();
	if(desc.Width == width && desc.Height == height) return;

	// リソースを再生成 (Initializeを再利用)
	Initialize(device.Get(), width, height, format, rtvHandle_, dsvHandle_);
}
