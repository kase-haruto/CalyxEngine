#include "GrayScale.h"
#include <Engine/PostProcess/FullscreenDrawer.h>

void GrayScaleEffect::Initialize(const PipelineSet& psoSet){
	psoSet_ = psoSet;
}

void GrayScaleEffect::Apply(ID3D12GraphicsCommandList* cmd,
							D3D12_GPU_DESCRIPTOR_HANDLE inputSRV,
							IRenderTarget* outputRT){
	outputRT->GetResource()->Transition(cmd, D3D12_RESOURCE_STATE_RENDER_TARGET);

	outputRT->SetRenderTarget(cmd);

	psoSet_.SetCommand(cmd);
	cmd->SetGraphicsRootDescriptorTable(0, inputSRV);

	cmd->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	cmd->DrawInstanced(3, 1, 0, 0);
}
