#include "CRTEffect.h"
#include <Engine/Graphics/Context/GraphicsGroup.h>
#include <Engine/Foundation/Clock/ClockManager.h>

void CRTEffect::Initialize(const PipelineSet& psoSet) {
	psoSet_ = psoSet;
	crtBuffer_.Initialize(GraphicsGroup::GetInstance()->GetDevice().Get());
}

void CRTEffect::Apply(ID3D12GraphicsCommandList* cmd,
					  D3D12_GPU_DESCRIPTOR_HANDLE inputSRV,
					  IRenderTarget* outputRT) {
	param_.time = ClockManager::GetInstance()->GetTotalTime(); // 秒単位

	// 描画リソース設定
	outputRT->GetResource()->Transition(cmd, D3D12_RESOURCE_STATE_RENDER_TARGET);
	crtBuffer_.TransferData(param_);
	outputRT->SetRenderTarget(cmd);

	psoSet_.SetCommand(cmd);
	cmd->SetGraphicsRootDescriptorTable(0, inputSRV); // t0
	crtBuffer_.SetCommand(cmd, 1);                    // b0

	cmd->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	cmd->DrawInstanced(3, 1, 0, 0);
}