#include "BloomEffect.h"

#include "Engine/Graphics/RenderTarget/OffscreenRT/OffscreenRenderTarget.h"

#include <Engine/Graphics/Context/GraphicsGroup.h>
#include <externals/imgui/imgui.h>

#include <algorithm>

void BloomEffect::Initialize(const PipelineSet& psoSet) {
	// Pipelineは外部Serviceが管理し、Effectは描画時に利用するSetと定数Bufferだけを保持する。
	psoSet_ = psoSet;
	buffer_.Initialize(GraphicsGroup::GetInstance()->GetDevice().Get());
	ResetParameters();
}

void BloomEffect::Apply(ID3D12GraphicsCommandList* cmd,
						D3D12_GPU_DESCRIPTOR_HANDLE inputSRV,
						IRenderTarget* outputRT) {
	// 出力TextureをRenderTarget状態へ遷移してからViewport/RTVをCommandListへ設定する。
	outputRT->GetResource()->Transition(cmd, D3D12_RESOURCE_STATE_RENDER_TARGET);
	outputRT->SetRenderTarget(cmd);

	// EditorまたはAnimationで変更されたParameterをDraw直前にGPUへ転送する。
	buffer_.TransferData(param_);
	psoSet_.SetCommand(cmd);

	auto* dxCore = GraphicsGroup::GetInstance()->GetDxCore();
	auto* offscreen = dxCore->GetRenderTargetCollection().Get("Offscreen");
	auto* offscreenRT = dynamic_cast<OffscreenRenderTarget*>(offscreen);

	if(offscreenRT) {
		// Bloom MaskをPixel Shaderから読むため、MRT Attachment 1をSRV状態へ遷移する。
		offscreenRT->TransitionMRTTo(cmd, 1, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
	}

	auto* maskResource = offscreenRT ? offscreenRT->GetMRTResource(1) : nullptr;
	// Maskを提供しないRender構成では不正DescriptorをBindingせず処理を中断する。
	if(!maskResource) {
		return;
	}
	D3D12_GPU_DESCRIPTOR_HANDLE maskSRV = maskResource->GetSRVGpuHandle();

	// Scene ColorとBloom Maskを所定のRoot ParameterへBindingしてFullscreen描画する。
	cmd->SetGraphicsRootDescriptorTable(0, inputSRV);
	cmd->SetGraphicsRootDescriptorTable(1, maskSRV);
	buffer_.SetCommand(cmd, 2);

	cmd->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	cmd->DrawInstanced(3, 1, 0, 0);
}

void BloomEffect::ShowImGui() {
	if(ImGui::CollapsingHeader("Bloom")) {
		ImGui::SliderFloat("Intensity", &param_.intensity, 0.0f, 5.0f);
		ImGui::SliderFloat("Threshold", &param_.threshold, -1.0f, 5.0f);
		ImGui::SliderFloat("Soft Knee", &param_.softKnee, 0.0f, 2.0f);
		ImGui::SliderFloat("Radius", &param_.radius, 0.0f, 4.0f);
		ImGui::ColorEdit3("Tint", &param_.tint.x);
		if(ImGui::Button("Reset")) {
			ResetParameters();
		}
	}
}

void BloomEffect::ResetParameters() {
	param_.intensity = 0.7f;
	param_.threshold = 1.0f;
	param_.softKnee = 0.5f;
	param_.radius = 1.0f;
	param_.tint = {1.0f, 1.0f, 1.0f};
	param_.padding = 0.0f;
}

nlohmann::json BloomEffect::SaveParameters() const {
	return nlohmann::json{
		{"intensity", param_.intensity},
		{"threshold", param_.threshold},
		{"softKnee", param_.softKnee},
		{"radius", param_.radius},
		{"tint", {param_.tint.x, param_.tint.y, param_.tint.z}}
	};
}

void BloomEffect::LoadParameters(const nlohmann::json& params) {
	// 各Keyの型を検証し、欠落値は現在Parameterを維持して旧Presetを読み込む。
	if(params.contains("intensity") && params["intensity"].is_number()) {
		SetIntensity(params["intensity"].get<float>());
	}
	if(params.contains("threshold") && params["threshold"].is_number()) {
		SetThreshold(params["threshold"].get<float>());
	}
	if(params.contains("softKnee") && params["softKnee"].is_number()) {
		SetSoftKnee(params["softKnee"].get<float>());
	}
	if(params.contains("radius") && params["radius"].is_number()) {
		SetRadius(params["radius"].get<float>());
	}
	// Tintは3要素配列だけを受理し、Shaderで安全なHDR色範囲へClampする。
	if(auto it = params.find("tint"); it != params.end() && it->is_array() && it->size() == 3) {
		param_.tint.x = std::clamp(it->at(0).get<float>(), 0.0f, 8.0f);
		param_.tint.y = std::clamp(it->at(1).get<float>(), 0.0f, 8.0f);
		param_.tint.z = std::clamp(it->at(2).get<float>(), 0.0f, 8.0f);
	}
}

bool BloomEffect::GetFloatParameter(const std::string& name, float& out) const {
	if(name == "intensity") {
		out = param_.intensity;
		return true;
	}
	if(name == "threshold") {
		out = param_.threshold;
		return true;
	}
	if(name == "softKnee") {
		out = param_.softKnee;
		return true;
	}
	if(name == "radius") {
		out = param_.radius;
		return true;
	}
	if(name == "tint.r") {
		out = param_.tint.x;
		return true;
	}
	if(name == "tint.g") {
		out = param_.tint.y;
		return true;
	}
	if(name == "tint.b") {
		out = param_.tint.z;
		return true;
	}
	return false;
}

bool BloomEffect::SetFloatParameter(const std::string& name, float value) {
	if(name == "intensity") {
		SetIntensity(value);
		return true;
	}
	if(name == "threshold") {
		SetThreshold(value);
		return true;
	}
	if(name == "softKnee") {
		SetSoftKnee(value);
		return true;
	}
	if(name == "radius") {
		SetRadius(value);
		return true;
	}
	if(name == "tint.r") {
		param_.tint.x = std::clamp(value, 0.0f, 8.0f);
		return true;
	}
	if(name == "tint.g") {
		param_.tint.y = std::clamp(value, 0.0f, 8.0f);
		return true;
	}
	if(name == "tint.b") {
		param_.tint.z = std::clamp(value, 0.0f, 8.0f);
		return true;
	}
	return false;
}

void BloomEffect::SetIntensity(float value) {
	param_.intensity = std::clamp(value, 0.0f, 10.0f);
}

void BloomEffect::SetThreshold(float value) {
	param_.threshold = std::clamp(value, -1.0f, 10.0f);
}

void BloomEffect::SetSoftKnee(float value) {
	param_.softKnee = std::clamp(value, 0.0f, 5.0f);
}

void BloomEffect::SetRadius(float value) {
	param_.radius = std::clamp(value, 0.0f, 8.0f);
}
