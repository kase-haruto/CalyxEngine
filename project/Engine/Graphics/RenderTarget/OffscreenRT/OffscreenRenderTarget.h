#pragma once
/* ========================================================================
/*	include space
/* ===================================================================== */
#include <Engine/Graphics/Descriptor/DescriptorAllocator.h>
#include <Engine/Graphics/GpuResource/DxGpuResource.h>
#include <Engine/Graphics/RenderTarget/Interface/IRenderTarget.h>

#include <memory>

class OffscreenRenderTarget
	: public IRenderTarget {
public:
	//===================================================================*/
	//		public methods
	//===================================================================*/
	OffscreenRenderTarget()			  = default;
	~OffscreenRenderTarget() override = default;
	void Initialize(ID3D12Device* device, uint32_t width, uint32_t height, DXGI_FORMAT format,
					DescriptorHandle rtvHandle,
					DescriptorHandle dsvHandle);

	DxGpuResource*				GetResource() const override;
	D3D12_CPU_DESCRIPTOR_HANDLE GetRTV() const override;
	D3D12_CPU_DESCRIPTOR_HANDLE GetDSV() const override;
	D3D12_GPU_DESCRIPTOR_HANDLE GetSRV() const override;
	D3D12_GPU_DESCRIPTOR_HANDLE GetDepthSRV() const override;
	D3D12_VIEWPORT				GetViewport() const override;
	void						SetDepthDSV(DescriptorHandle dsv) override;
	D3D12_RECT					GetScissorRect() const override;
	void						Clear(ID3D12GraphicsCommandList* cmdList) override;
	void						SetRenderTarget(ID3D12GraphicsCommandList* commandList) override;

	void			 SetRenderTargetType(RenderTargetType type) override { rtType_ = type; }
	RenderTargetType GetRenderTargetType() const { return rtType_; }

	void TransitionTo(ID3D12GraphicsCommandList* cmdList, D3D12_RESOURCE_STATES newState) override;
	void TransitionDepthTo(ID3D12GraphicsCommandList* cmdList, D3D12_RESOURCE_STATES newState) override;
	void Resize(uint32_t width, uint32_t height) override;

private:
	//===================================================================*/
	//		private variables
	//===================================================================*/
	RenderTargetType			   rtType_;
	std::unique_ptr<DxGpuResource> resource_;
	std::unique_ptr<DxGpuResource> depthResource_;
	DescriptorHandle			   rtvHandle_{};
	DescriptorHandle			   dsvHandle_{};

	D3D12_VIEWPORT viewport_{};
	D3D12_RECT	   scissorRect_{};
};
