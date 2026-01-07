#include "PipelineStateObject.h"

#include <Engine/Graphics/Context/GraphicsGroup.h>

void PipelineStateObject::SetRootSignature(ID3D12RootSignature* root) {
	rootSignature_ = root;
}

bool PipelineStateObject::Initialize(const D3D12_GRAPHICS_PIPELINE_STATE_DESC& desc) {
	ID3D12Device* device = GraphicsGroup::GetInstance()->GetDevice().Get();
	HRESULT hr = device->CreateGraphicsPipelineState(&desc, IID_PPV_ARGS(&pipelineState_));
	if (FAILED(hr)) {
		char buf[128];
		sprintf_s(buf, "CreateGraphicsPipelineState failed. hr=0x%08X\n", (unsigned)hr);
		OutputDebugStringA(buf);
		return false;
	}
	return true;
}

bool PipelineStateObject::Initialize(const D3D12_COMPUTE_PIPELINE_STATE_DESC& desc) {
	ID3D12Device* device = GraphicsGroup::GetInstance()->GetDevice().Get();
	HRESULT hr = device->CreateComputePipelineState(&desc, IID_PPV_ARGS(&pipelineState_));
	if (FAILED(hr)) {
		OutputDebugStringA("Failed to create compute pipeline state\n");
		return false;
	}
	isCompute_ = true;
	return true;
}