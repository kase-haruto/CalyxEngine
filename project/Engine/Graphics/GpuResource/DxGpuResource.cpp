#include "DxGpuResource.h"
/* ========================================================================
/*	include space
/* ===================================================================== */
// engine
#include <Engine/Graphics/Descriptor/DescriptorAllocator.h>

// c++
#include <stdexcept>

void DxGpuResource::InitializeAsRenderTarget(ID3D12Device*				 device,
											 uint32_t					 width,
											 uint32_t					 height,
											 DXGI_FORMAT				 format,
											 std::optional<std::wstring> name,
											 const float*				 clearColor) {
	D3D12_RESOURCE_DESC texDesc = {};
	texDesc.Dimension			= D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	texDesc.Width				= width;
	texDesc.Height				= height;
	texDesc.DepthOrArraySize	= 1;
	texDesc.MipLevels			= 1;
	texDesc.Format				= format;
	texDesc.SampleDesc.Count	= 1;
	texDesc.Layout				= D3D12_TEXTURE_LAYOUT_UNKNOWN;
	texDesc.Flags				= D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;

	D3D12_HEAP_PROPERTIES heapProps = {};
	heapProps.Type					= D3D12_HEAP_TYPE_DEFAULT;

	D3D12_CLEAR_VALUE clearValue = {};
	clearValue.Format			 = format;
	if(clearColor) {
		clearValue.Color[0] = clearColor[0];
		clearValue.Color[1] = clearColor[1];
		clearValue.Color[2] = clearColor[2];
		clearValue.Color[3] = clearColor[3];
	} else {
		clearValue.Color[0] = 0.1f;
		clearValue.Color[1] = 0.1f;
		clearValue.Color[2] = 0.1f;
		clearValue.Color[3] = 1.0f;
	}

	HRESULT hr = device->CreateCommittedResource(
		&heapProps,
		D3D12_HEAP_FLAG_NONE,
		&texDesc,
		D3D12_RESOURCE_STATE_RENDER_TARGET,
		&clearValue,
		IID_PPV_ARGS(&resource_));

	if(FAILED(hr)) {
		throw std::runtime_error("Failed to create render target resource.");
	}

	if(name.has_value()) {
		resource_->SetName(name->c_str());
	}
}

void DxGpuResource::InitializeAsDepthStencil(ID3D12Device*				 device,
											 uint32_t					 width,
											 uint32_t					 height,
											 DXGI_FORMAT				 format,
											 std::optional<std::wstring> name) {
	D3D12_RESOURCE_DESC texDesc = {};
	texDesc.Dimension			= D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	texDesc.Width				= width;
	texDesc.Height				= height;
	texDesc.DepthOrArraySize	= 1;
	texDesc.MipLevels			= 1;
	texDesc.Format				= format;
	texDesc.SampleDesc.Count	= 1;
	texDesc.Layout				= D3D12_TEXTURE_LAYOUT_UNKNOWN;
	texDesc.Flags				= D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

	D3D12_HEAP_PROPERTIES heapProps = {};
	heapProps.Type					= D3D12_HEAP_TYPE_DEFAULT;

	D3D12_CLEAR_VALUE clearValue = {};
	if(format == DXGI_FORMAT_R24G8_TYPELESS) {
		clearValue.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	} else if(format == DXGI_FORMAT_R32_TYPELESS) {
		clearValue.Format = DXGI_FORMAT_D32_FLOAT;
	} else {
		clearValue.Format = format;
	}
	clearValue.DepthStencil.Depth	= 1.0f;
	clearValue.DepthStencil.Stencil = 0;

	HRESULT hr = device->CreateCommittedResource(
		&heapProps,
		D3D12_HEAP_FLAG_NONE,
		&texDesc,
		D3D12_RESOURCE_STATE_DEPTH_WRITE,
		&clearValue,
		IID_PPV_ARGS(&resource_));

	if(FAILED(hr)) {
		throw std::runtime_error("Failed to create depth stencil resource.");
	}

	if(name.has_value()) {
		resource_->SetName(name->c_str());
	}
	currentState_ = D3D12_RESOURCE_STATE_DEPTH_WRITE;
}

void DxGpuResource::SetCurrentState(D3D12_RESOURCE_STATES state) {
	currentState_ = state;
}

D3D12_RESOURCE_STATES DxGpuResource::GetCurrentState() const {
	return currentState_;
}

void DxGpuResource::Transition(ID3D12GraphicsCommandList* cmdList, D3D12_RESOURCE_STATES newState) {
	if(!resource_ || currentState_ == newState) return;

	D3D12_RESOURCE_BARRIER barrier{};
	barrier.Type				   = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier.Transition.pResource   = resource_.Get();
	barrier.Transition.StateBefore = currentState_;
	barrier.Transition.StateAfter  = newState;
	barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

	cmdList->ResourceBarrier(1, &barrier);
	currentState_ = newState;
}

void DxGpuResource::CreateSRV(ID3D12Device* device) {
	DescriptorHandle handle = DescriptorAllocator::Allocate(DescriptorUsage::CbvSrvUav);
	cpuSrvHandle_			= handle.cpu;
	gpuSrvHandle_			= handle.gpu;

	// SRV 設定
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Shader4ComponentMapping			= D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

	DXGI_FORMAT format = resource_->GetDesc().Format;
	if(format == DXGI_FORMAT_R24G8_TYPELESS) {
		srvDesc.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
	} else if(format == DXGI_FORMAT_R32_TYPELESS) {
		srvDesc.Format = DXGI_FORMAT_R32_FLOAT;
	} else {
		srvDesc.Format = format;
	}

	srvDesc.ViewDimension				  = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MostDetailedMip	  = 0;
	srvDesc.Texture2D.MipLevels			  = 1;
	srvDesc.Texture2D.PlaneSlice		  = 0;
	srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;

	device->CreateShaderResourceView(resource_.Get(), &srvDesc, cpuSrvHandle_);
}

void DxGpuResource::CreateRTV(ID3D12Device* device, D3D12_CPU_DESCRIPTOR_HANDLE handle) {
	cpuRtvHandle_ = handle;

	device->CreateRenderTargetView(resource_.Get(), nullptr, handle);
}

void DxGpuResource::CreateDSV(ID3D12Device* device, D3D12_CPU_DESCRIPTOR_HANDLE handle) {
	cpuDsvHandle_ = handle;

	D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
	DXGI_FORMAT					  format  = resource_->GetDesc().Format;
	if(format == DXGI_FORMAT_R24G8_TYPELESS) {
		dsvDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	} else if(format == DXGI_FORMAT_R32_TYPELESS) {
		dsvDesc.Format = DXGI_FORMAT_D32_FLOAT;
	} else {
		dsvDesc.Format = format;
	}
	dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
	dsvDesc.Flags		  = D3D12_DSV_FLAG_NONE;

	device->CreateDepthStencilView(resource_.Get(), &dsvDesc, handle);
}

void DxGpuResource::UpdateSRV(ID3D12Device* device) {
	if(!cpuSrvHandle_.ptr) return;

	// SRV 設定
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Shader4ComponentMapping			= D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

	DXGI_FORMAT format = resource_->GetDesc().Format;
	if(format == DXGI_FORMAT_R24G8_TYPELESS) {
		srvDesc.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
	} else if(format == DXGI_FORMAT_R32_TYPELESS) {
		srvDesc.Format = DXGI_FORMAT_R32_FLOAT;
	} else {
		srvDesc.Format = format;
	}

	srvDesc.ViewDimension				  = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MostDetailedMip	  = 0;
	srvDesc.Texture2D.MipLevels			  = 1;
	srvDesc.Texture2D.PlaneSlice		  = 0;
	srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;

	device->CreateShaderResourceView(resource_.Get(), &srvDesc, cpuSrvHandle_);
}
