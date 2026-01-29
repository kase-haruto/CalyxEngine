#pragma once
#include <Engine/Application/Effects/Particle/Emitter/FxEmitter.h>
#include <Engine/Application/Effects/Particle/Emitter/GpuFxEmitter.h>
#include <Engine/Graphics/Buffer/DxStructuredBuffer.h>

#include <d3d12.h>
#include <memory>
#include <vector>
#include <string>

class PipelineService;
struct MeshResource;

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
	/*
	 * \brief モデルが描画可能な状態か確認し、準備ができていなければ準備する
	 * \param mesh meshデータ
	 * \param device D3D12デバイス
	 */
	void EnsureMeshIsReady(MeshResource& mesh, ID3D12Device* device);
	/*
	 * \brief インスタンス描画を行う
	 * \param mesh メッシュデータ
	 * \param cmdList コマンドリスト
	 * \param instanceCount インスタンス数
	 * \param handle インスタンス行列バッファのSRVハンドル
	 */
	void DrawMeshInstanced(MeshResource& mesh,
							ID3D12GraphicsCommandList* cmdList,
							UINT instanceCount,
							D3D12_GPU_DESCRIPTOR_HANDLE handle);

private:
	DxStructuredBuffer<CalyxEffect::ParticleConstantData> instanceBuffer_;
};