#pragma once
/* ========================================================================
/*	include space
/* ===================================================================== */
#include <Engine/Graphics/Descriptor/DescriptorAllocator.h>
#include <Engine/Graphics/GpuResource/DxGpuResource.h>
#include <Engine/Graphics/RenderTarget/Interface/IRenderTarget.h>

#include <memory>

/**
 * @brief OffscreenRenderTargetの機能を提供するクラスです。
 */
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
	void InitializeMRT(ID3D12Device*                        device,uint32_t             width,uint32_t height,const std::vector<DXGI_FORMAT>& formats,
						   const std::vector<DescriptorHandle>& rtvHandles,DescriptorHandle dsvHandle);

	DxGpuResource*				GetResource() const override;
	D3D12_CPU_DESCRIPTOR_HANDLE GetRTV() const override;
	D3D12_CPU_DESCRIPTOR_HANDLE GetDSV() const override;
	D3D12_GPU_DESCRIPTOR_HANDLE GetSRV() const override;
	D3D12_GPU_DESCRIPTOR_HANDLE GetDepthSRV() const override;
	D3D12_VIEWPORT				GetViewport() const override;
	void						SetDepthDSV(DescriptorHandle dsv) override;
	D3D12_RECT					GetScissorRect() const override;
	void Clear(ID3D12GraphicsCommandList* cmdList) override;
	void SetRenderTarget(ID3D12GraphicsCommandList* commandList) override;
	void SetRenderTargetMRT(ID3D12GraphicsCommandList* commandList);
	void TransitionMRTTo(ID3D12GraphicsCommandList* cmdList, size_t index, D3D12_RESOURCE_STATES newState);

	void			 SetRenderTargetType(RenderTargetType type) override { rtType_ = type; }
	RenderTargetType GetRenderTargetType() const { return rtType_; }

	void TransitionTo(ID3D12GraphicsCommandList* cmdList, D3D12_RESOURCE_STATES newState) override;
	void TransitionDepthTo(ID3D12GraphicsCommandList* cmdList, D3D12_RESOURCE_STATES newState) override;
	void Resize(uint32_t width, uint32_t height) override;
		// MRT用メソッド
	DxGpuResource* GetMRTResource(size_t index) const { return index < mrtResources_.size() ? mrtResources_[index].get() : nullptr; }

private:
	//===================================================================*/
	//		private variables
	//===================================================================*/
	RenderTargetType			   rtType_;
	std::unique_ptr<DxGpuResource> resource_;
	std::unique_ptr<DxGpuResource> depthResource_;
	DescriptorHandle			   rtvHandle_{};
	DescriptorHandle			   dsvHandle_{};

	// MRT対応
	std::vector<std::unique_ptr<DxGpuResource>> mrtResources_; // 複数RTリソース
	std::vector<DescriptorHandle>   mrtRtvHandles_{};  // 複数RTVハンドル

	D3D12_VIEWPORT viewport_{};
	D3D12_RECT	   scissorRect_{};
};
