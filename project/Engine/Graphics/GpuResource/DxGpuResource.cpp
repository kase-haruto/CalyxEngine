#include "DxGpuResource.h"
/* ========================================================================
/*	include space
/* ===================================================================== */
// engine
#include <Engine/Graphics/Descriptor/DescriptorAllocator.h>

// c++
#include <stdexcept>
#include <sstream>

namespace {
	std::runtime_error MakeResourceCreateError(const char* kind, HRESULT hr, uint32_t width, uint32_t height, DXGI_FORMAT format) {
		std::ostringstream oss;
		oss << "Failed to create " << kind << " resource. HRESULT=0x"
			<< std::hex << static_cast<unsigned long>(hr)
			<< std::dec << " size=" << width << "x" << height
			<< " format=" << static_cast<int>(format);
		return std::runtime_error(oss.str());
	}
}

void DxGpuResource::InitializeAsRenderTarget(ID3D12Device*				 device,
											 uint32_t					 width,
											 uint32_t					 height,
											 DXGI_FORMAT				 format,
											 std::optional<std::wstring> name,
											 const float*				 clearColor) {
	// Shader出力先として利用する2D TextureのResource記述を構築する。
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

	// Optimized Clear ValueをResource作成時に指定し、RenderTarget Clearを効率化する。
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

	// Resize時は旧COM Resourceを解放してから同じWrapperへ新Resourceを格納する。
	resource_.Reset();
	HRESULT hr = device->CreateCommittedResource(
		&heapProps,
		D3D12_HEAP_FLAG_NONE,
		&texDesc,
		D3D12_RESOURCE_STATE_RENDER_TARGET,
		&clearValue,
		IID_PPV_ARGS(&resource_));

	if(FAILED(hr)) {
		throw MakeResourceCreateError("render target", hr, width, height, format);
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

	// Typeless Resourceには対応するDSV FormatをClear Valueとして指定する。
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

	// Depth Textureを初期状態DEPTH_WRITEで生成し、直後のDepth PassでBarrierを不要にする。
	resource_.Reset();
	HRESULT hr = device->CreateCommittedResource(
		&heapProps,
		D3D12_HEAP_FLAG_NONE,
		&texDesc,
		D3D12_RESOURCE_STATE_DEPTH_WRITE,
		&clearValue,
		IID_PPV_ARGS(&resource_));

	if(FAILED(hr)) {
		throw MakeResourceCreateError("depth stencil", hr, width, height, format);
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
	// Resource未生成または同一状態の場合は冗長なBarrierを記録しない。
	if(!resource_ || currentState_ == newState) return;

	D3D12_RESOURCE_BARRIER barrier{};
	barrier.Type				   = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier.Transition.pResource   = resource_.Get();
	barrier.Transition.StateBefore = currentState_;
	barrier.Transition.StateAfter  = newState;
	barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

	// 全Subresourceを遷移させ、Command記録後に追跡状態を新Stateへ同期する。
	cmdList->ResourceBarrier(1, &barrier);
	currentState_ = newState;
}

void DxGpuResource::CreateSRV(ID3D12Device* device) {
	// Descriptorの所有権はAllocatorへ残し、CPU/GPU HandleだけをResourceと関連付ける。
	DescriptorHandle handle = DescriptorAllocator::Allocate(DescriptorUsage::CbvSrvUav);
	cpuSrvHandle_			= handle.cpu;
	gpuSrvHandle_			= handle.gpu;

	// SRV 設定
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Shader4ComponentMapping			= D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

	// Typeless Depth TextureはShaderから読める型付きFormatへ変換してSRVを作成する。
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
	// 外部Allocatorが確保したRTV Slotへ現在ResourceのViewを書き込む。
	cpuRtvHandle_ = handle;

	device->CreateRenderTargetView(resource_.Get(), nullptr, handle);
}

void DxGpuResource::CreateDSV(ID3D12Device* device, D3D12_CPU_DESCRIPTOR_HANDLE handle) {
	cpuDsvHandle_ = handle;

	// Typeless ResourceをDepth書込み用の型付きFormatとして公開する。
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
	// Resize前から保持するDescriptor Slotがある場合だけ、新ResourceへViewを差し替える。
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
