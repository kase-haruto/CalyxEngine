#include "OffscreenRenderTarget.h"
#include <Engine/Application/System/Enviroment.h>

#include <cassert>
#include <d3dx12.h>
#include <stdexcept>

void OffscreenRenderTarget::Initialize(ID3D12Device* device, uint32_t width, uint32_t height, DXGI_FORMAT format, D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle, D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle){
	rtvHandle_ = rtvHandle;
	dsvHandle_ = dsvHandle;

	// リソースの初期化
	viewport_ = {0.0f, 0.0f, static_cast< float >(width), static_cast< float >(height), 0.0f, 1.0f};
	scissorRect_ = {0, 0, static_cast< LONG >(width), static_cast< LONG >(height)};

	resource_ = std::make_unique<DxGpuResource>();
	resource_->InitializeAsRenderTarget(device, width, height, format);
	resource_->CreateRTV(device, rtvHandle);
	resource_->CreateSRV(device);

	// 初期状態を保存（RENDER_TARGET）
	resource_->SetCurrentState(D3D12_RESOURCE_STATE_RENDER_TARGET);

	// Depthバッファ作成
	D3D12_RESOURCE_DESC depthDesc = {};
	depthDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	depthDesc.Width = width;
	depthDesc.Height = height;
	depthDesc.DepthOrArraySize = 1;
	depthDesc.MipLevels = 1;
	depthDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	depthDesc.SampleDesc.Count = 1;
	depthDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	depthDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

	D3D12_CLEAR_VALUE clearValue = {};
	clearValue.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	clearValue.DepthStencil.Depth = 1.0f;
	clearValue.DepthStencil.Stencil = 0;

	CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_DEFAULT);
	HRESULT hr = device->CreateCommittedResource(
		&heapProps,
		D3D12_HEAP_FLAG_NONE,
		&depthDesc,
		D3D12_RESOURCE_STATE_DEPTH_WRITE,
		&clearValue,
		IID_PPV_ARGS(&depthBuffer_)
	);
	if (FAILED(hr)){
		throw std::runtime_error("Failed to create depth buffer");
	}

	D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
	dsvDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
	dsvDesc.Flags = D3D12_DSV_FLAG_NONE;

	device->CreateDepthStencilView(depthBuffer_.Get(), &dsvDesc, dsvHandle);
}

DxGpuResource* OffscreenRenderTarget::GetResource() const{
	return resource_.get();
}

D3D12_CPU_DESCRIPTOR_HANDLE OffscreenRenderTarget::GetRTV() const{
	return rtvHandle_;
}

D3D12_CPU_DESCRIPTOR_HANDLE OffscreenRenderTarget::GetDSV() const{
	return dsvHandle_;
}

D3D12_GPU_DESCRIPTOR_HANDLE OffscreenRenderTarget::GetSRV() const{
	return resource_->GetSRVGpuHandle();
}

D3D12_VIEWPORT OffscreenRenderTarget::GetViewport() const{
	return viewport_;
}

D3D12_RECT OffscreenRenderTarget::GetScissorRect() const{
	return scissorRect_;
}

void OffscreenRenderTarget::Clear(ID3D12GraphicsCommandList* commandList){
	resource_->Transition(commandList, D3D12_RESOURCE_STATE_RENDER_TARGET);
	float clearColor[] = {0.1f, 0.1f, 0.1f, 1.0f};
	commandList->ClearRenderTargetView(rtvHandle_, clearColor, 0, nullptr);
	commandList->ClearDepthStencilView(dsvHandle_, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
}

void OffscreenRenderTarget::SetRenderTarget(ID3D12GraphicsCommandList* commandList) {
	commandList->RSSetViewports(1, &viewport_);
	commandList->RSSetScissorRects(1, &scissorRect_);
	commandList->OMSetRenderTargets(1, &rtvHandle_, FALSE, &dsvHandle_);
}

void OffscreenRenderTarget::TransitionTo(ID3D12GraphicsCommandList* cmdList, D3D12_RESOURCE_STATES newState){
	resource_->Transition(cmdList, newState);
}
