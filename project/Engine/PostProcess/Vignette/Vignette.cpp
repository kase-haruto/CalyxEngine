#include "Vignette.h"

#include <Engine/PostProcess/FullscreenDrawer.h>
#include <Engine/Graphics/Context/GraphicsGroup.h>
#include <externals/imgui/imgui.h>
#include <algorithm>

void Vignette::Initialize(const PipelineSet& psoSet) {
	psoSet_ = psoSet;
	buffer_.Initialize(GraphicsGroup::GetInstance()->GetDevice().Get());

	ResetParameters();
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
	if(ImGui::CollapsingHeader("Vignette")) {
		ImGui::SliderFloat("Strength", &param_.strength, 0.0f, 1.0f);
		ImGui::SliderFloat("Radius", &param_.radius, 0.0f, 1.0f);
		ImGui::ColorEdit3("Color", &param_.color.x);
		if(ImGui::Button("Reset")) ResetParameters();
	}
}

void Vignette::ResetParameters() {
	param_.strength = 1.0f;
	param_.radius = 0.0f;
	param_.color = {0.0f, 0.0f, 0.0f};
}

nlohmann::json Vignette::SaveParameters() const {
	return nlohmann::json{
		{"strength", param_.strength},
		{"radius", param_.radius},
		{"color", {param_.color.x, param_.color.y, param_.color.z}}
	};
}

void Vignette::LoadParameters(const nlohmann::json& params) {
	if(params.contains("strength") && params["strength"].is_number()) {
		param_.strength = std::clamp(params["strength"].get<float>(), 0.0f, 1.0f);
	}
	if(params.contains("radius") && params["radius"].is_number()) {
		param_.radius = std::clamp(params["radius"].get<float>(), 0.0f, 1.0f);
	}
	if(params.contains("color") && params["color"].is_array() && params["color"].size() >= 3) {
		param_.color.x = std::clamp(params["color"][0].get<float>(), 0.0f, 1.0f);
		param_.color.y = std::clamp(params["color"][1].get<float>(), 0.0f, 1.0f);
		param_.color.z = std::clamp(params["color"][2].get<float>(), 0.0f, 1.0f);
	}
}

bool Vignette::GetFloatParameter(const std::string& name, float& out) const {
	if(name == "strength") {
		out = param_.strength;
		return true;
	}
	if(name == "radius") {
		out = param_.radius;
		return true;
	}
	if(name == "color.r") { out = param_.color.x; return true; }
	if(name == "color.g") { out = param_.color.y; return true; }
	if(name == "color.b") { out = param_.color.z; return true; }
	return false;
}

bool Vignette::SetFloatParameter(const std::string& name, float value) {
	if(name == "strength") {
		param_.strength = std::clamp(value, 0.0f, 1.0f);
		return true;
	}
	if(name == "radius") {
		param_.radius = std::clamp(value, 0.0f, 1.0f);
		return true;
	}
	if(name == "color.r") { param_.color.x = std::clamp(value, 0.0f, 1.0f); return true; }
	if(name == "color.g") { param_.color.y = std::clamp(value, 0.0f, 1.0f); return true; }
	if(name == "color.b") { param_.color.z = std::clamp(value, 0.0f, 1.0f); return true; }
	return false;
}
