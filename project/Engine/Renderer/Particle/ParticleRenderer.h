#pragma once
#include <Engine/Application/Effects/Particle/Emitter/FxEmitter.h>
#include <Engine/Application/Effects/Particle/Emitter/GpuFxEmitter.h>
#include <Engine/Graphics/Buffer/DxStructuredBuffer.h>
#include <Engine/Graphics/Buffer/DxVertexBuffer.h>
#include <Engine/Graphics/Buffer/DxIndexBuffer.h>
#include <Engine/Graphics/Buffer/DxConstantBuffer.h>
#include <Engine/Application/Effects/Trail/TrailRuntime.h>

#include <d3d12.h>
#include <memory>
#include <vector>
#include <string>
#include <unordered_map>

class PipelineService;
struct MeshResource;

/**
 * @brief ParticleRendererの機能を提供するクラスです。
 */
class ParticleRenderer{
public:
	// ── CPU と GPU を一緒に描画 ─────────────────────
	void Render(const std::vector<std::shared_ptr<CalyxEngine::FxEmitter>>& cpuEmitters,
				const std::vector<std::shared_ptr<CalyxEngine::GpuFxEmitter>>& gpuEmitters,
				PipelineService* pipelineService,
				ID3D12GraphicsCommandList* cmdList);
	void ClearTrailResources() { trailStates_.clear(); trailMeshScratch_.Clear(); }

	// （CPU 用のまとめ描きユーティリティは残す）
	void RenderGrouped(const std::string& modelPath,
					   const std::vector<CalyxEngine::ParticleConstantData>& gpuUnits,
					   ID3D12GraphicsCommandList* cmdList);

private:
	/**
	 * @brief TrailGpuMaterialに関するデータを保持する構造体です。
	 */
	struct alignas(16) TrailGpuMaterial {
		CalyxEngine::Vector4 color{1,1,1,1};
		CalyxEngine::Vector4 baseTilingScroll{};   //< xy tiling, zw scroll
		CalyxEngine::Vector4 noiseTilingScroll{};  //< xy tiling, zw scroll
		CalyxEngine::Vector4 noiseDistortion{};    //< x attached, y strength, z distortion, w time
		CalyxEngine::Vector4 dissolve{};           //< x enabled, y start, z end, w softness
		CalyxEngine::Vector4 dissolveEdge{};       //< x width, y emissive, zw padding
		CalyxEngine::Vector4 dissolveEdgeColor{1,1,1,1};
		CalyxEngine::Vector4 fadeClip{};           //< x head, y tail, z alpha clip
		CalyxEngine::Vector4 emissiveColorIntensity{1,1,1,0};
	};

	/**
	 * @brief TrailRenderStateに関するデータを保持する構造体です。
	 */
	struct TrailRenderState {
		DxVertexBuffer<CalyxEngine::TrailVertex> vertexBuffer;
		DxIndexBuffer<uint32_t> indexBuffer;
		DxConstantBuffer<TrailGpuMaterial> materialBuffer;
		uint32_t vertexCapacity = 0;
		uint32_t indexCapacity = 0;
		Guid baseGuid{Guid::Empty()};
		Guid noiseGuid{Guid::Empty()};
		std::string basePath;
		std::string noisePath;
		D3D12_GPU_DESCRIPTOR_HANDLE baseTexture{};
		D3D12_GPU_DESCRIPTOR_HANDLE noiseTexture{};
	};

	void RenderTrails(const std::vector<std::shared_ptr<CalyxEngine::FxEmitter>>& emitters,
		PipelineService* pipelineService,ID3D12GraphicsCommandList* cmdList);
	static uint32_t NextPowerOfTwo(uint32_t value);

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
	void DrawGpuBillboards(ID3D12GraphicsCommandList* cmdList,
						   UINT instanceCount,
						   D3D12_GPU_DESCRIPTOR_HANDLE particleHandle);

private:
	DxStructuredBuffer<CalyxEngine::ParticleConstantData> instanceBuffer_;
	std::unordered_map<const CalyxEngine::FxEmitter*,TrailRenderState> trailStates_;
	CalyxEngine::TrailMeshData trailMeshScratch_;
};
