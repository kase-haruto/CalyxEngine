#include "BlendEffect.h"

#include <Engine/Graphics/Context/GraphicsGroup.h>
#include <externals/imgui/imgui.h>

#include <algorithm>

void BlendEffect::Initialize(const PipelineSet& psoSet) {
	psoSet_ = psoSet;
	buffer_.Initialize(GraphicsGroup::GetInstance()->GetDevice().Get());
	ResetParameters();
}

void BlendEffect::Apply(ID3D12GraphicsCommandList* cmd,
						D3D12_GPU_DESCRIPTOR_HANDLE inputSRV,
						IRenderTarget* outputRT) {
	Apply(cmd, inputSRV, inputSRV, outputRT);
}

void BlendEffect::Apply(ID3D12GraphicsCommandList* cmd,
						D3D12_GPU_DESCRIPTOR_HANDLE inputA,
						D3D12_GPU_DESCRIPTOR_HANDLE inputB,
						IRenderTarget* outputRT) {
	outputRT->GetResource()->Transition(cmd, D3D12_RESOURCE_STATE_RENDER_TARGET);
	outputRT->SetRenderTarget(cmd);

	buffer_.TransferData(param_);
	psoSet_.SetCommand(cmd);
	cmd->SetGraphicsRootDescriptorTable(0, inputA);
	cmd->SetGraphicsRootDescriptorTable(1, inputB);
	buffer_.SetCommand(cmd, 2);

	cmd->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	cmd->DrawInstanced(3, 1, 0, 0);
}

void BlendEffect::ShowImGui() {
	if(ImGui::CollapsingHeader("Blend")) {
		ImGui::SliderFloat("Opacity", &param_.opacity, 0.0f, 1.0f);
		int mode = static_cast<int>(param_.mode);
		if(ImGui::Combo("Mode", &mode, "Lerp\0Add\0Multiply\0Max\0")) {
			param_.mode = static_cast<float>(mode);
		}
		if(ImGui::Button("Reset")) ResetParameters();
	}
}

void BlendEffect::ResetParameters() {
	param_.opacity = 0.5f;
	param_.mode = 0.0f;
}

nlohmann::json BlendEffect::SaveParameters() const {
	return nlohmann::json{{"opacity", param_.opacity}, {"mode", static_cast<int>(param_.mode)}};
}

void BlendEffect::LoadParameters(const nlohmann::json& params) {
	if(params.contains("opacity") && params["opacity"].is_number()) {
		param_.opacity = std::clamp(params["opacity"].get<float>(), 0.0f, 1.0f);
	}
	if(params.contains("mode") && params["mode"].is_number_integer()) {
		param_.mode = static_cast<float>(std::clamp(params["mode"].get<int>(), 0, 3));
	}
}

bool BlendEffect::GetFloatParameter(const std::string& name, float& out) const {
	if(name == "opacity") {
		out = param_.opacity;
		return true;
	}
	if(name == "mode") {
		out = param_.mode;
		return true;
	}
	return false;
}

bool BlendEffect::SetFloatParameter(const std::string& name, float value) {
	if(name == "opacity") {
		param_.opacity = std::clamp(value, 0.0f, 1.0f);
		return true;
	}
	if(name == "mode") {
		param_.mode = static_cast<float>(std::clamp(static_cast<int>(value), 0, 3));
		return true;
	}
	return false;
}
