#include "PostEffectGraph.h"
#include <Engine/PostProcess/Collection/PostProcessCollection.h>
#include <Engine/Graphics/RenderTarget/Interface/IRenderTarget.h>
#include <Engine/Graphics/Device/DxCore.h>
#include <cassert>

void PostEffectGraph::SetPassesFromList(const std::vector<PostEffectSlot>& slots){
	passes_.clear();
	int enabledCount = 0;

	for (const auto& slot : slots){
		if (slot.enabled && slot.pass){
			++enabledCount;
			passes_.push_back(slot.pass);
		}
	}

	/*if (enabledCount > 1){
		assert(enabledCount <= 2 && "PostEffectGraph currently supports only up to 2 enabled passes for ping-pong rendering.");
	}*/
}

void PostEffectGraph::Execute(ID3D12GraphicsCommandList* cmd,
							  DxGpuResource* input,
							  IRenderTarget* finalTarget,
							  CalyxGraphics::DxCore* dxCore){




	input->Transition(cmd, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

	// Ping-Pongバッファ
	auto rt1 = dxCore->GetRenderTargetCollection().Get("PostEffectBuffer1");
	auto rt2 = dxCore->GetRenderTargetCollection().Get("PostEffectBuffer2");

	IRenderTarget* currentOutput = rt1;
	bool useFirstRT = true;
	D3D12_GPU_DESCRIPTOR_HANDLE currentSRV = input->GetSRVGpuHandle();

	for (size_t i = 0; i < passes_.size(); ++i){
		currentOutput->GetResource()->Transition(cmd, D3D12_RESOURCE_STATE_RENDER_TARGET);
		passes_[i]->Apply(cmd, currentSRV, currentOutput);
		currentOutput->GetResource()->Transition(cmd, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

		currentSRV = currentOutput->GetSRV();

		// Ping-Pong切り替え
		useFirstRT = !useFirstRT;
		currentOutput = useFirstRT ? rt1 : rt2;
	}

	// 最終出力にPostProcessCollectionのCopyImageを使用してコピー
	finalTarget->GetResource()->Transition(cmd, D3D12_RESOURCE_STATE_RENDER_TARGET);
	postProcessCollection_->GetEffectByName("CopyImage")->Apply(cmd, currentSRV, finalTarget);
}