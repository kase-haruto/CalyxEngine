#include "FilmGrainEffect.h"

#include <Engine/Graphics/Context/GraphicsGroup.h>
#include <externals/imgui/imgui.h>

#include <algorithm>
#include <cmath>

void FilmGrainEffect::Initialize(const PipelineSet& psoSet) {
	psoSet_ = psoSet;
	buffer_.Initialize(GraphicsGroup::GetInstance()->GetDevice().Get());
	ResetParameters();
}

void FilmGrainEffect::Apply(ID3D12GraphicsCommandList* cmd,
	D3D12_GPU_DESCRIPTOR_HANDLE inputSRV,
	IRenderTarget* outputRT) {
	outputRT->GetResource()->Transition(cmd, D3D12_RESOURCE_STATE_RENDER_TARGET);
	buffer_.TransferData(param_);
	outputRT->SetRenderTarget(cmd);
	psoSet_.SetCommand(cmd);
	cmd->SetGraphicsRootDescriptorTable(0, inputSRV);
	buffer_.SetCommand(cmd, 1);
	cmd->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	cmd->DrawInstanced(3, 1, 0, 0);
}

void FilmGrainEffect::Tick(float dt) {
	param_.time = std::fmod(param_.time + (std::max)(dt, 0.0f) * param_.speed, 4096.0f);
}

void FilmGrainEffect::ShowImGui() {
	if(ImGui::CollapsingHeader("Film Grain")) {
		ImGui::SliderFloat("Intensity", &param_.intensity, 0.0f, 0.5f);
		ImGui::SliderFloat("Grain Size", &param_.grainSize, 0.5f, 8.0f);
		ImGui::SliderFloat("Speed", &param_.speed, 0.0f, 10.0f);
		if(ImGui::Button("Reset")) ResetParameters();
	}
}

void FilmGrainEffect::ResetParameters() {
	param_.intensity = 0.08f;
	param_.grainSize = 1.5f;
	param_.speed = 1.0f;
	param_.time = 0.0f;
}

nlohmann::json FilmGrainEffect::SaveParameters() const {
	return {{"intensity", param_.intensity}, {"grainSize", param_.grainSize}, {"speed", param_.speed}};
}

void FilmGrainEffect::LoadParameters(const nlohmann::json& params) {
	if(params.contains("intensity") && params["intensity"].is_number()) SetFloatParameter("intensity", params["intensity"].get<float>());
	if(params.contains("grainSize") && params["grainSize"].is_number()) SetFloatParameter("grainSize", params["grainSize"].get<float>());
	if(params.contains("speed") && params["speed"].is_number()) SetFloatParameter("speed", params["speed"].get<float>());
}

bool FilmGrainEffect::GetFloatParameter(const std::string& name, float& out) const {
	if(name == "intensity") out = param_.intensity;
	else if(name == "grainSize") out = param_.grainSize;
	else if(name == "speed") out = param_.speed;
	else return false;
	return true;
}

bool FilmGrainEffect::SetFloatParameter(const std::string& name, float value) {
	if(name == "intensity") param_.intensity = std::clamp(value, 0.0f, 0.5f);
	else if(name == "grainSize") param_.grainSize = std::clamp(value, 0.5f, 8.0f);
	else if(name == "speed") param_.speed = std::clamp(value, 0.0f, 10.0f);
	else return false;
	return true;
}
