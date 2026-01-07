#pragma once
#include "Engine/Graphics/Device/DxCore.h"

#include <Engine/Graphics/GpuResource/DxGpuResource.h>
#include <Engine/PostProcess/Interface/IPostEffectPass.h>
#include <Engine/PostProcess/Slot/PostEffectSlot.h>
#include <vector>

namespace CalyxGraphics {
	class CalyxGraphics::DxCore;
}
class PostProcessCollection;
class IRenderTarget;

class PostEffectGraph{
public:
	PostEffectGraph(PostProcessCollection* postProcessCollection)
		: postProcessCollection_(postProcessCollection){}

	void SetPassesFromList(const std::vector<PostEffectSlot>& slots);

	void Execute(ID3D12GraphicsCommandList* cmd,
				 DxGpuResource* input,
				 IRenderTarget* finalTarget,
				 CalyxGraphics::DxCore* dxCore);

private:
	std::vector<IPostEffectPass*> passes_;
	PostProcessCollection* postProcessCollection_ = nullptr;
};