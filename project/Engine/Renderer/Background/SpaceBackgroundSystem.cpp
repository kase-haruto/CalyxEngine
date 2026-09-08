#include "SpaceBackgroundSystem.h"

#include <Engine/Graphics/Camera/Base/BaseCamera.h>
#include <Engine/Graphics/Pipeline/Service/PipelineService.h>
#include <Engine/Graphics/RenderTarget/Interface/IRenderTarget.h>
#include <externals/imgui/imgui.h>
#include <algorithm>

namespace {
	template<class TSettings, class TBuffer>
	void DrawBackground(ID3D12GraphicsCommandList* cmd, const PipelineSet& pipeline,
		IRenderTarget* target, BaseCamera* camera, TSettings& settings, TBuffer& buffer) {
		if(!cmd || !target || !camera) return;
		target->SetRenderTarget(cmd);
		pipeline.SetCommand(cmd);
		camera->SetRootCommand(cmd, 0);
		buffer.TransferData(settings);
		buffer.SetCommand(cmd, 1);
		cmd->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		cmd->DrawInstanced(3, 1, 0, 0);
	}
}

SpaceBackgroundSystem* SpaceBackgroundSystem::Get() {
	static SpaceBackgroundSystem instance;
	return &instance;
}

void NebulaBackgroundPass::Initialize(ID3D12Device* device) { buffer_.Initialize(device); }
void NebulaBackgroundPass::Render(ID3D12GraphicsCommandList* cmd, PipelineService* p, IRenderTarget* rt, BaseCamera* camera) {
	DrawBackground(cmd, p->GetPipelineSet(PipelineTag::Background::Nebula), rt, camera, settings_, buffer_);
}
void StarFieldRenderer::Initialize(ID3D12Device* device) { buffer_.Initialize(device); }
void StarFieldRenderer::Render(ID3D12GraphicsCommandList* cmd, PipelineService* p, IRenderTarget* rt, BaseCamera* camera) {
	DrawBackground(cmd, p->GetPipelineSet(PipelineTag::Background::StarField), rt, camera, settings_, buffer_);
}
void ForegroundGlowRenderer::Initialize(ID3D12Device* device) { buffer_.Initialize(device); }
void ForegroundGlowRenderer::Render(ID3D12GraphicsCommandList* cmd, PipelineService* p, IRenderTarget* rt, BaseCamera* camera) {
	DrawBackground(cmd, p->GetPipelineSet(PipelineTag::Background::ForegroundGlow), rt, camera, settings_, buffer_);
}
void SpaceDustSystem::Initialize(ID3D12Device* device) { buffer_.Initialize(device); }
void SpaceDustSystem::Render(ID3D12GraphicsCommandList* cmd, PipelineService* p, IRenderTarget* rt, BaseCamera* camera) {
	DrawBackground(cmd, p->GetPipelineSet(PipelineTag::Background::SpaceDust), rt, camera, settings_, buffer_);
}

void SpaceBackgroundSystem::Initialize(ID3D12Device* device) {
	if(initialized_ || !device) return;
	nebula_.Initialize(device); stars_.Initialize(device); glow_.Initialize(device); dust_.Initialize(device);
	initialized_ = true;
}
void SpaceBackgroundSystem::Update(float dt) {
	dt = std::clamp(dt, 0.0f, 0.1f);
	nebula_.Settings().time += dt;
	stars_.Settings().time += dt;
	glow_.Settings().moveSpeedIntensity.x += dt;
	dust_.Settings().time += dt;
}
void SpaceBackgroundSystem::RenderFar(ID3D12GraphicsCommandList* cmd, PipelineService* p, IRenderTarget* rt, BaseCamera* camera) {
	nebula_.Render(cmd, p, rt, camera);
	stars_.Render(cmd, p, rt, camera);
}
void SpaceBackgroundSystem::RenderNear(ID3D12GraphicsCommandList* cmd, PipelineService* p, IRenderTarget* rt, BaseCamera* camera) {
	glow_.Render(cmd, p, rt, camera);
	dust_.Render(cmd, p, rt, camera);
}

void SpaceBackgroundSystem::ShowImGui() {
	if(!ImGui::CollapsingHeader("Space Background")) return;
	auto& n = nebula_.Settings();
	ImGui::ColorEdit3("Primary Color", &n.primaryColor.x); ImGui::ColorEdit3("Secondary Color", &n.secondaryColor.x);
	ImGui::ColorEdit3("Accent Color", &n.accentColor.x); ImGui::SliderFloat("Noise Scale", &n.noiseScale, 0.2f, 8.0f);
	ImGui::SliderFloat("Noise Strength", &n.noiseStrength, 0.0f, 2.0f); ImGui::SliderFloat("Warp Scale", &n.warpScale, 0.1f, 4.0f);
	ImGui::SliderFloat("Warp Strength", &n.warpStrength, 0.0f, 4.0f); ImGui::SliderFloat("Animation Speed", &n.animationSpeed, 0.0f, 1.0f);
	ImGui::SliderFloat("Nebula Brightness", &n.brightness, 0.0f, 1.0f); ImGui::SliderFloat("Nebula Emission", &n.emission, 0.0f, 0.25f);
	ImGui::SliderFloat("Nebula Parallax", &n.parallaxStrength, 0.0f, 0.2f);
	auto& s = stars_.Settings();
	ImGui::SliderFloat("Star Density", &s.density, 0.0f, 0.3f); ImGui::SliderFloat("Star Min Size", &s.minSize, 0.002f, 0.04f);
	ImGui::SliderFloat("Star Max Size", &s.maxSize, 0.01f, 0.12f); ImGui::SliderFloat("Star Brightness", &s.brightness, 0.0f, 5.0f);
	ImGui::SliderFloat("Twinkle Speed", &s.twinkleSpeed, 0.0f, 3.0f); ImGui::SliderFloat("Twinkle Strength", &s.twinkleStrength, 0.0f, 1.0f);
	ImGui::SliderFloat("Star Bloom", &s.bloomIntensity, 0.0f, 5.0f); ImGui::SliderFloat("Star Parallax", &s.parallaxStrength, 0.0f, 0.3f);
	auto& g = glow_.Settings();
	ImGui::ColorEdit3("Glow Color 0", &g.color0.x); ImGui::ColorEdit3("Glow Color 1", &g.color1.x); ImGui::ColorEdit3("Glow Color 2", &g.color2.x);
	ImGui::SliderFloat2("Glow Position 0", &g.positionRadius0.x, 0.0f, 1.0f); ImGui::SliderFloat("Glow Radius 0", &g.positionRadius0.z, 0.05f, 1.5f);
	ImGui::SliderFloat2("Glow Position 1", &g.positionRadius1.x, 0.0f, 1.0f); ImGui::SliderFloat("Glow Radius 1", &g.positionRadius1.z, 0.05f, 1.5f);
	ImGui::SliderFloat2("Glow Position 2", &g.positionRadius2.x, 0.0f, 1.0f); ImGui::SliderFloat("Glow Radius 2", &g.positionRadius2.z, 0.05f, 1.5f);
	ImGui::SliderFloat("Glow Move", &g.moveSpeedIntensity.y, 0.0f, 0.15f);
	ImGui::SliderFloat("Glow Intensity", &g.moveSpeedIntensity.z, 0.0f, 0.2f);
	auto& d = dust_.Settings();
	ImGui::SliderFloat("Dust Density", &d.density, 0.0f, 0.15f); ImGui::SliderFloat("Dust Size", &d.size, 0.01f, 0.2f);
	ImGui::SliderFloat("Dust Speed", &d.speed, 0.0f, 0.5f); ImGui::SliderFloat("Dust Alpha", &d.alpha, 0.0f, 0.2f);
	ImGui::SliderFloat("Dust Parallax", &d.parallaxStrength, 0.0f, 0.5f);
}
