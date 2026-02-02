#pragma once

#include <Engine/Graphics/Descriptor/DescriptorAllocator.h>
#include <Engine/Graphics/GpuResource/DxGpuResource.h>
#include <Engine/Graphics/RenderTarget/Detail/RenderTargetDetail.h>
#include <d3d12.h>


class IRenderTarget {
public:
	virtual ~IRenderTarget()																	= default;
	virtual DxGpuResource*				GetResource() const										= 0;
	virtual D3D12_CPU_DESCRIPTOR_HANDLE GetRTV() const											= 0;
	virtual D3D12_CPU_DESCRIPTOR_HANDLE GetDSV() const											= 0;
	virtual D3D12_GPU_DESCRIPTOR_HANDLE GetSRV() const											= 0;
	virtual D3D12_GPU_DESCRIPTOR_HANDLE GetDepthSRV() const										= 0;
	virtual D3D12_VIEWPORT				GetViewport() const										= 0;
	virtual void						SetDepthDSV(DescriptorHandle dsv)						= 0;
	virtual D3D12_RECT					GetScissorRect() const									= 0;
	virtual void						SetRenderTargetType(RenderTargetType type)				= 0;
	virtual RenderTargetType			GetRenderTargetType() const								= 0;
	virtual void						SetRenderTarget(ID3D12GraphicsCommandList* commandList) = 0;
	virtual void						TransitionTo([[maybe_unused]] ID3D12GraphicsCommandList* cmdList, [[maybe_unused]] D3D12_RESOURCE_STATES newState) {};
	virtual void						Clear(ID3D12GraphicsCommandList* cmdList) = 0;

	/**
	 * \brief リサイズ処理
	 * \param width 幅
	 * \param height 高さ
	 */
	virtual void Resize(uint32_t width, uint32_t height) = 0;
};
