#include "ChromaticAberrationEffect.h"
#include <Engine/Graphics/Context/GraphicsGroup.h>
#include <algorithm> // clamp

void ChromaticAberrationEffect::Initialize(const PipelineSet& psoSet){
	psoSet_ = psoSet;
	settingsBuffer_.Initialize(GraphicsGroup::GetInstance()->GetDevice().Get());
	ResetParameters(); 
}

void ChromaticAberrationEffect::Apply(ID3D12GraphicsCommandList* cmd,
									  D3D12_GPU_DESCRIPTOR_HANDLE inputSRV,
									  IRenderTarget* outputRT){
	ChromaticAberrationSettings s {};
	s.intensity = intensity_;
	settingsBuffer_.TransferData(s);

	outputRT->GetResource()->Transition(cmd, D3D12_RESOURCE_STATE_RENDER_TARGET);
	outputRT->SetRenderTarget(cmd);

	psoSet_.SetCommand(cmd);
	// RootParameter 0: input SRV / 1: CB
	cmd->SetGraphicsRootDescriptorTable(0, inputSRV);
	settingsBuffer_.SetCommand(cmd, 1);

	cmd->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	cmd->DrawInstanced(3, 1, 0, 0);
}

void ChromaticAberrationEffect::SetIntensity(float intensity){
	intensity_ = std::clamp(intensity, kMin, kMax);
}

void ChromaticAberrationEffect::ResetParameters(){
	intensity_ = 0.0f;
}

void ChromaticAberrationEffect::ShowImGui(){
	if (ImGui::CollapsingHeader("Chromatic Aberration")){
		ImGui::SliderFloat("Intensity", &intensity_, kMin, kMax);
		ImGui::SameLine();
		if (ImGui::Button("Reset")) ResetParameters();
	}
}

nlohmann::json ChromaticAberrationEffect::SaveParameters() const{
	return nlohmann::json{{"intensity", intensity_}};
}

void ChromaticAberrationEffect::LoadParameters(const nlohmann::json& params){
	if(params.contains("intensity") && params["intensity"].is_number()){
		SetIntensity(params["intensity"].get<float>());
	}
}

bool ChromaticAberrationEffect::GetFloatParameter(const std::string& name, float& out) const{
	if(name == "intensity"){
		out = intensity_;
		return true;
	}
	return false;
}

bool ChromaticAberrationEffect::SetFloatParameter(const std::string& name, float value){
	if(name == "intensity"){
		SetIntensity(value);
		return true;
	}
	return false;
}
