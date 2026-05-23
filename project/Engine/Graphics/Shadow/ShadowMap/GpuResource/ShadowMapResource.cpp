#include "ShadowMapResource.h"

#include "Engine/Graphics/Descriptor/DescriptorAllocator.h"

namespace CalyxEngine {

	void ShadowMapResource::Initialize(
		ID3D12Device* device,
		uint32_t      width,
		uint32_t      height) {
		// -----------------------------
		// Resource
		// -----------------------------
		D3D12_RESOURCE_DESC desc{};
		desc.Dimension        = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
		desc.Width            = width;
		desc.Height           = height;
		desc.DepthOrArraySize = 1;
		desc.MipLevels        = 1;
		desc.Format			  = DXGI_FORMAT_R32_TYPELESS;
		desc.SampleDesc.Count = 1;
		desc.Layout           = D3D12_TEXTURE_LAYOUT_UNKNOWN;
		desc.Flags            = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

		D3D12_CLEAR_VALUE clear{};
		clear.Format               = DXGI_FORMAT_D32_FLOAT;
		clear.DepthStencil.Depth   = 1.0f;
		clear.DepthStencil.Stencil = 0;

		D3D12_HEAP_PROPERTIES heap{};
		heap.Type = D3D12_HEAP_TYPE_DEFAULT;

		device->CreateCommittedResource(
			&heap,
			D3D12_HEAP_FLAG_NONE,
			&desc,
			D3D12_RESOURCE_STATE_DEPTH_WRITE,
			&clear,
			IID_PPV_ARGS(resource_.ReleaseAndGetAddressOf())
			);

		// -----------------------------
		// DSV
		// -----------------------------
		{
			auto handle = DescriptorAllocator::Allocate(DescriptorUsage::Dsv);
			dsv_        = handle.cpu;
			D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc{};
			dsvDesc.Format        = DXGI_FORMAT_D32_FLOAT;
			dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
			dsvDesc.Flags         = D3D12_DSV_FLAG_NONE;
			dsvDesc.Texture2D.MipSlice = 0;

			device->CreateDepthStencilView(
				resource_.Get(),
				&dsvDesc,
				dsv_
			);
		}

		// -----------------------------
		// SRV (D32 → R32)
		// -----------------------------
		{
			auto handle = DescriptorAllocator::Allocate(DescriptorUsage::CbvSrvUav);
			srvCpu_     = handle.cpu;
			srvGpu_     = handle.gpu;

			D3D12_SHADER_RESOURCE_VIEW_DESC srv{};
			srv.Format                  = DXGI_FORMAT_R32_FLOAT;
			srv.ViewDimension           = D3D12_SRV_DIMENSION_TEXTURE2D;
			srv.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
			srv.Texture2D.MipLevels     = 1;

			device->CreateShaderResourceView(resource_.Get(),&srv,srvCpu_);
		}

		// -----------------------------
		// Viewport / Scissor
		// -----------------------------
		viewport_ = {
				0.0f,0.0f,
				static_cast<float>(width),
				static_cast<float>(height),
				0.0f,1.0f
			};

		scissor_ = {
				0,0,
				static_cast<LONG>(width),
				static_cast<LONG>(height)
			};
	}

	void ShadowMapResource::BeginShadowPass(ID3D12GraphicsCommandList* cmdList) {
		if(currentState_ != D3D12_RESOURCE_STATE_DEPTH_WRITE) {
			D3D12_RESOURCE_BARRIER barrier{};
			barrier.Type                   = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
			barrier.Transition.pResource   = resource_.Get();
			barrier.Transition.StateBefore = currentState_;
			barrier.Transition.StateAfter  = D3D12_RESOURCE_STATE_DEPTH_WRITE;
			barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
			cmdList->ResourceBarrier(1,&barrier);
			currentState_ = D3D12_RESOURCE_STATE_DEPTH_WRITE;
		}

		cmdList->RSSetViewports(1,&viewport_);
		cmdList->RSSetScissorRects(1,&scissor_);

		cmdList->OMSetRenderTargets(0,nullptr,false,&dsv_);

		cmdList->ClearDepthStencilView(
			dsv_,
			D3D12_CLEAR_FLAG_DEPTH,
			1.0f,
			0,
			0,
			nullptr
			);
	}

	void ShadowMapResource::EndShadowPass(ID3D12GraphicsCommandList* cmdList) {
		D3D12_RESOURCE_BARRIER barrier{};
		barrier.Type                   = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		barrier.Transition.pResource   = resource_.Get();
		barrier.Transition.StateBefore = currentState_;
		barrier.Transition.StateAfter  = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
		barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

		cmdList->ResourceBarrier(1,&barrier);
		currentState_ = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
	}

}