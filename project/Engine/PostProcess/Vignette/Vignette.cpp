#include "Vignette.h"

#include <Engine/PostProcess/FullscreenDrawer.h>
#include <Engine/Graphics/Context/GraphicsGroup.h>
#include <externals/imgui/imgui.h>

void Vignette::Initialize(const PipelineSet& psoSet) {
	psoSet_ = psoSet;
	buffer_.Initialize(GraphicsGroup::GetInstance()->GetDevice().Get());

	param_.strength = 1.0f;
	param_.radius = 0.0f;
}

void Vignette::Apply(ID3D12GraphicsCommandList* cmd,
					 D3D12_GPU_DESCRIPTOR_HANDLE inputSRV,
					 IRenderTarget* outputRT) {
	outputRT->GetResource()->Transition(cmd, D3D12_RESOURCE_STATE_RENDER_TARGET);

	// 定数バッファ転送
	buffer_.TransferData(param_);

	outputRT->SetRenderTarget(cmd);
	psoSet_.SetCommand(cmd);

	cmd->SetGraphicsRootDescriptorTable(0, inputSRV); // t0
	buffer_.SetCommand(cmd, 1); // b0

	cmd->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	cmd->DrawInstanced(3, 1, 0, 0);
}

void Vignette::ShowImGui() {
	//ImGui::SliderFloat("Vignette Strength", &param_.strength, 0.0f, 1.0f);
	//ImGui::SliderFloat("Vignette Radius", &param_.radius, 0.0f, 1.0f);
}