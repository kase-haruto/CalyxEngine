#pragma once

#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl.h>
#include <cassert>

namespace{
	using Microsoft::WRL::ComPtr;
}

ComPtr<ID3D12DescriptorHeap> CreateDescriptorHeap(
	ComPtr<ID3D12Device> device,
	D3D12_DESCRIPTOR_HEAP_TYPE heapType,
	UINT numDescriptors,
	bool shaderVisible);

void DrawTextureToRenderTarget(
	ID3D12GraphicsCommandList* cmd,
	D3D12_GPU_DESCRIPTOR_HANDLE inputSRV,
	class IRenderTarget* output,
	ID3D12PipelineState* pso,
	ID3D12RootSignature* rootSig
);