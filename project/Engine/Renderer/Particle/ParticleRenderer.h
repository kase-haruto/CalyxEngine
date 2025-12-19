#pragma once
#include <Engine/Application/Effects/Particle/Emitter/FxEmitter.h>
#include <Engine/Application/Effects/Particle/Emitter/GpuFxEmitter.h>
#include <Engine/Graphics/Buffer/DxStructuredBuffer.h>

#include <d3d12.h>
#include <memory>
#include <vector>
#include <string>

class PipelineService;
struct ModelData;

class ParticleRenderer{
public:
	// ── CPU と GPU を一緒に描画 ─────────────────────
	void Render(const std::vector<std::shared_ptr<CalyxEffect::FxEmitter>>& cpuEmitters,
				const std::vector<std::shared_ptr<CalyxEffect::GpuFxEmitter>>& gpuEmitters,
				PipelineService* pipelineService,
				ID3D12GraphicsCommandList* cmdList);

	// （CPU 用のまとめ描きユーティリティは残す）
	void RenderGrouped(const std::string& modelPath,
					   const std::vector<CalyxEffect::ParticleConstantData>& gpuUnits,
					   ID3D12GraphicsCommandList* cmdList);

private:
	void EnsureModelIsReady(ModelData& model, ID3D12Device* device);
	void DrawModelInstanced(ModelData& model,
							ID3D12GraphicsCommandList* cmdList,
							UINT instanceCount,
							D3D12_GPU_DESCRIPTOR_HANDLE handle);

private:
	DxStructuredBuffer<CalyxEffect::ParticleConstantData> instanceBuffer_;
};