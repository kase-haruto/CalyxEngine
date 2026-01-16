#pragma once
/* ========================================================================
/*	include space
/* ===================================================================== */
#include <Engine/Graphics/RenderTarget/Interface/IRenderTarget.h>
#include <Engine/Graphics/SwapChain/DxSwapChain.h>
#include <Engine/Graphics/Descriptor/DescriptorAllocator.h>

class SwapChainRenderTarget 
	: public IRenderTarget{
public:
	//===================================================================*/
	//			public methods
	//===================================================================*/
	void Initialize(DxSwapChain* swapChain, ID3D12DescriptorHeap* rtvHeap, UINT rtvDescriptorSize);
	void SetBufferIndex(UINT index);

	DxGpuResource* GetResource() const override{ return nullptr; };
	D3D12_CPU_DESCRIPTOR_HANDLE GetRTV() const override;
	D3D12_CPU_DESCRIPTOR_HANDLE GetDSV() const override{ return dsv_; }
	D3D12_GPU_DESCRIPTOR_HANDLE GetSRV() const override;
	void SetRenderTarget(ID3D12GraphicsCommandList* commandList) override;
	void TransitionTo(ID3D12GraphicsCommandList* commandList, D3D12_RESOURCE_STATES newState) override;
	void Clear(ID3D12GraphicsCommandList* cmdList) override;
	void Resize(uint32_t width, uint32_t height) override;

	D3D12_VIEWPORT GetViewport() const override{ return viewport_; }
	D3D12_RECT GetScissorRect() const override{ return scissorRect_; }

	void SetRenderTargetType(RenderTargetType type) override{ rtType_ = type; }
	RenderTargetType GetRenderTargetType() const{ return rtType_; }

	void ReleaseSRVs();
	void SetDepthDSV(D3D12_CPU_DESCRIPTOR_HANDLE dsv)override { dsv_ = dsv; }
private:
	//===================================================================*/
	//			private variables
	//===================================================================*/
	RenderTargetType rtType_ {};
	DxSwapChain* swapChain_ = nullptr;
	ID3D12DescriptorHeap* rtvHeap_ = nullptr;
	UINT rtvDescriptorSize_ = 0;
	UINT bufferIndex_ = 0;

	D3D12_CPU_DESCRIPTOR_HANDLE dsv_{};
	std::vector<D3D12_RESOURCE_STATES> currentStates_;
	std::vector<DescriptorHandle> srvHandles_;

	D3D12_VIEWPORT viewport_ {};
	D3D12_RECT scissorRect_ {};
};