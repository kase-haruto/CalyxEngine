#include "CRTEffect.h"
#include <Engine/Graphics/Context/GraphicsGroup.h>

void CRTEffect::Initialize(const PipelineSet& psoSet) {
	psoSet_ = psoSet;
	crtBuffer_.Initialize(GraphicsGroup::GetInstance()->GetDevice().Get());
}

void CRTEffect::Apply(ID3D12GraphicsCommandList* cmd,
					  D3D12_GPU_DESCRIPTOR_HANDLE inputSRV,
					  IRenderTarget* outputRT) {
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

nlohmann::json CRTEffect::SaveParameters() const {
	return nlohmann::json{{"screenSize", {param_.screenSize.x, param_.screenSize.y}}};
}

void CRTEffect::LoadParameters(const nlohmann::json& params) {
	if(auto it = params.find("screenSize"); it != params.end() && it->is_array() && it->size() == 2) {
		param_.screenSize = {it->at(0).get<float>(), it->at(1).get<float>()};
	}
}
