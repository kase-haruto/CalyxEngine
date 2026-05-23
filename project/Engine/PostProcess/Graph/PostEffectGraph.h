#pragma once
#include "Engine/Graphics/Device/DxCore.h"

#include <Engine/Graphics/GpuResource/DxGpuResource.h>
#include <Engine/PostProcess/Interface/IPostEffectPass.h>
#include <Engine/PostProcess/Slot/PostEffectSlot.h>
#include <externals/nlohmann/json.hpp>
#include <string>
#include <unordered_map>
#include <vector>

namespace CalyxEngine {
	class CalyxEngine::DxCore;
}
class PostProcessCollection;
class IRenderTarget;

/*-----------------------------------------------------------------------------------------
 * PostEffectGraph
 * - ポストエフェクトグラフクラス
 * - 複数のポストエフェクトパスを連結して実行するパイプラインを管理
 *---------------------------------------------------------------------------------------*/
class PostEffectGraph{
public:
	PostEffectGraph(PostProcessCollection* postProcessCollection)
		: postProcessCollection_(postProcessCollection){}

	void SetPassesFromList(const std::vector<PostEffectSlot>& slots);
	void SetGraphFromJson(const nlohmann::json& root, const std::vector<PostEffectSlot>& slots);

	void Execute(ID3D12GraphicsCommandList* cmd,
				 DxGpuResource* input,
				 IRenderTarget* finalTarget,
				 CalyxEngine::DxCore* dxCore);

private:
	struct GraphNode {
		int32_t id = 0;
		std::string type;
		std::vector<int32_t> inputPins;
		std::vector<int32_t> outputPins;
		IPostEffectPass* pass = nullptr;
	};

	D3D12_GPU_DESCRIPTOR_HANDLE ExecuteGraphNode(ID3D12GraphicsCommandList* cmd,
												 int32_t nodeId,
												 D3D12_GPU_DESCRIPTOR_HANDLE sceneSRV,
												 CalyxEngine::DxCore* dxCore,
												 std::unordered_map<int32_t, D3D12_GPU_DESCRIPTOR_HANDLE>& cache,
												 std::unordered_map<int32_t, bool>& visiting,
												 int& tempIndex);
	IRenderTarget* AcquireTempTarget(CalyxEngine::DxCore* dxCore, int& tempIndex) const;
	int32_t FindInputSourceNode(int32_t inputPinId) const;
	int32_t FindOutputSourceNode() const;

	std::vector<IPostEffectPass*> passes_;
	PostProcessCollection* postProcessCollection_ = nullptr;
	bool useNodeGraph_ = false;
	std::unordered_map<int32_t, GraphNode> graphNodes_;
	std::unordered_map<int32_t, int32_t> pinOwner_;
	std::unordered_map<int32_t, int32_t> inputPinToSourceNode_;
	int32_t outputNodeId_ = 0;
};
