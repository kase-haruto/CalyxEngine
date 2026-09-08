#pragma once

#include <Engine/Foundation/Math/Vector4.h>
#include <Engine/Graphics/Buffer/DxConstantBuffer.h>
#include <Engine/Graphics/Pipeline/Pso/PsoDetails.h>

class IRenderTarget;
class PipelineService;
class BaseCamera;

struct NebulaSettings {
	CalyxEngine::Vector4 primaryColor{0.18f, 0.04f, 0.34f, 1.0f};
	CalyxEngine::Vector4 secondaryColor{0.025f, 0.16f, 0.42f, 1.0f};
	CalyxEngine::Vector4 accentColor{0.55f, 0.08f, 0.42f, 1.0f};
	float noiseScale = 2.35f;
	float noiseStrength = 0.82f;
	float warpScale = 0.72f;
	float warpStrength = 0.85f;
	// 星雲全体を画面上で移動させず、ドメインワープだけを非常にゆっくり循環させる。
	float animationSpeed = 0.008f;
	float brightness = 0.42f;
	float emission = 0.06f;
	float parallaxStrength = 0.018f;
	float time = 0.0f;
	float padding[3]{};
};

struct StarFieldSettings {
	float density = 0.085f;
	float minSize = 0.035f;
	float maxSize = 0.105f;
	float brightness = 1.45f;
	float twinkleSpeed = 0.75f;
	float twinkleStrength = 0.18f;
	float bloomIntensity = 1.8f;
	float parallaxStrength = 0.055f;
	float time = 0.0f;
	float padding[3]{};
};

struct ForegroundGlowSettings {
	CalyxEngine::Vector4 color0{0.28f, 0.05f, 0.42f, 1.0f};
	CalyxEngine::Vector4 color1{0.02f, 0.20f, 0.42f, 1.0f};
	CalyxEngine::Vector4 color2{0.48f, 0.04f, 0.30f, 1.0f};
	CalyxEngine::Vector4 positionRadius0{0.12f, 0.22f, 0.62f, 0.32f};
	CalyxEngine::Vector4 positionRadius1{0.88f, 0.75f, 0.72f, 0.25f};
	CalyxEngine::Vector4 positionRadius2{0.55f, 0.08f, 0.48f, 0.18f};
	CalyxEngine::Vector4 moveSpeedIntensity{0.0f, 0.005f, 0.055f, 0.0f};
};

struct SpaceDustSettings {
	float density = 0.035f;
	float size = 0.085f;
	float speed = 0.06f;
	float alpha = 0.08f;
	float parallaxStrength = 0.18f;
	float time = 0.0f;
	float padding[2]{};
};

class NebulaBackgroundPass {
public:
	void Initialize(ID3D12Device* device);
	void Render(ID3D12GraphicsCommandList* cmd, PipelineService* pipelines, IRenderTarget* target, BaseCamera* camera);
	NebulaSettings& Settings() { return settings_; }
private:
	NebulaSettings settings_{};
	DxConstantBuffer<NebulaSettings> buffer_{};
};

class StarFieldRenderer {
public:
	void Initialize(ID3D12Device* device);
	void Render(ID3D12GraphicsCommandList* cmd, PipelineService* pipelines, IRenderTarget* target, BaseCamera* camera);
	StarFieldSettings& Settings() { return settings_; }
private:
	StarFieldSettings settings_{};
	DxConstantBuffer<StarFieldSettings> buffer_{};
};

class ForegroundGlowRenderer {
public:
	void Initialize(ID3D12Device* device);
	void Render(ID3D12GraphicsCommandList* cmd, PipelineService* pipelines, IRenderTarget* target, BaseCamera* camera);
	ForegroundGlowSettings& Settings() { return settings_; }
private:
	ForegroundGlowSettings settings_{};
	DxConstantBuffer<ForegroundGlowSettings> buffer_{};
};

class SpaceDustSystem {
public:
	void Initialize(ID3D12Device* device);
	void Render(ID3D12GraphicsCommandList* cmd, PipelineService* pipelines, IRenderTarget* target, BaseCamera* camera);
	SpaceDustSettings& Settings() { return settings_; }
private:
	SpaceDustSettings settings_{};
	DxConstantBuffer<SpaceDustSettings> buffer_{};
};

class SpaceBackgroundSystem {
public:
	static SpaceBackgroundSystem* Get();
	void Initialize(ID3D12Device* device);
	void Update(float deltaTime);
	void RenderFar(ID3D12GraphicsCommandList* cmd, PipelineService* pipelines, IRenderTarget* target, BaseCamera* camera);
	void RenderNear(ID3D12GraphicsCommandList* cmd, PipelineService* pipelines, IRenderTarget* target, BaseCamera* camera);
	void ShowImGui();

private:
	NebulaBackgroundPass nebula_{};
	StarFieldRenderer stars_{};
	ForegroundGlowRenderer glow_{};
	SpaceDustSystem dust_{};
	bool initialized_ = false;
};
