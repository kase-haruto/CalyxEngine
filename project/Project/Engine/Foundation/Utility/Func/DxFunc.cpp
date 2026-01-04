#include "DxFunc.h"

#include <Engine/Graphics/RenderTarget/Interface/IRenderTarget.h>

ComPtr<ID3D12DescriptorHeap> CreateDescriptorHeap(
	ComPtr<ID3D12Device> device,
	D3D12_DESCRIPTOR_HEAP_TYPE heapType,
	UINT numDescriptors,
	bool shaderVisible){

	ComPtr<ID3D12DescriptorHeap> descriptorHeap = nullptr;
	D3D12_DESCRIPTOR_HEAP_DESC descriptorHeapDesc {};
	descriptorHeapDesc.Type = heapType;
	descriptorHeapDesc.NumDescriptors = numDescriptors;
	descriptorHeapDesc.Flags = shaderVisible ? D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE : D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	HRESULT hr = device->CreateDescriptorHeap(&descriptorHeapDesc, IID_PPV_ARGS(&descriptorHeap));
	assert(SUCCEEDED(hr));
	// Releaseビルドで未使用警告を抑える
	( void ) hr;
	return descriptorHeap;
}

void DrawTextureToRenderTarget(
	ID3D12GraphicsCommandList* cmd,
	D3D12_GPU_DESCRIPTOR_HANDLE inputSRV,
	IRenderTarget* output,
	ID3D12PipelineState* pso,
	ID3D12RootSignature* rootSig
){
	if (!cmd || !output || !pso || !rootSig) return;

	output->SetRenderTarget(cmd);

	cmd->SetPipelineState(pso);
	cmd->SetGraphicsRootSignature(rootSig);
	cmd->SetGraphicsRootDescriptorTable(0, inputSRV);

	cmd->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	cmd->DrawInstanced(3, 1, 0, 0);
}
