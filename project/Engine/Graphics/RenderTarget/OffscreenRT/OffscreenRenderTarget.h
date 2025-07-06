#pragma once
/* ========================================================================
/*	include space
/* ===================================================================== */
#include <Engine/Graphics/RenderTarget/Interface/IRenderTarget.h>
#include <Engine/Graphics/GpuResource/DxGpuResource.h>

#include <memory>

class OffscreenRenderTarget
	: public IRenderTarget{
public:
	//===================================================================*/
	//		public methods
	//===================================================================*/
	void Initialize(ID3D12Device* device, uint32_t width, uint32_t height, DXGI_FORMAT format,
					D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle,
					D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle);

	DxGpuResource* GetResource() const override;
	D3D12_CPU_DESCRIPTOR_HANDLE GetRTV() const override;
	D3D12_CPU_DESCRIPTOR_HANDLE GetDSV() const override;
	D3D12_GPU_DESCRIPTOR_HANDLE GetSRV() const override;
	D3D12_VIEWPORT GetViewport() const override;
	D3D12_RECT GetScissorRect() const override;
	void Clear(ID3D12GraphicsCommandList* cmdList) override;
	void SetRenderTarget(ID3D12GraphicsCommandList* commandList)override;

	void SetRenderTargetType(RenderTargetType type)override { rtType_ = type; }
	RenderTargetType GetRenderTargetType()const { return rtType_; }

	void TransitionTo(ID3D12GraphicsCommandList* cmdList, D3D12_RESOURCE_STATES newState);

private:
	//===================================================================*/
	//		private variables
	//===================================================================*/
	RenderTargetType rtType_;
	std::unique_ptr<DxGpuResource> resource_;
	Microsoft::WRL::ComPtr<ID3D12Resource> depthBuffer_;
	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle_ {};
	D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle_ {};

	D3D12_VIEWPORT viewport_ {};
	D3D12_RECT scissorRect_ {};
};
