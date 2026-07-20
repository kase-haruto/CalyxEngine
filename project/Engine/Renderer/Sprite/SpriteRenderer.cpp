#include "SpriteRenderer.h"

#include <Engine/Graphics/Pipeline/Presets/PipelinePresets.h>
#include <Engine/Graphics/Pipeline/Service/PipelineService.h>
#include <Engine/Renderer/Sprite/Sprite.h>

#include <algorithm>

void SpriteRenderer::Register(Sprite* sprite) {
	Register(sprite, kDefaultSortingLayerId, 0, nextSubmissionOrder_);
}

void SpriteRenderer::Register(Sprite* sprite, SortingLayerId layerId, int32_t orderInLayer, uint64_t stableOrder) {
	if(!sprite) return;
	sprites_.push_back({
		sprite,
		SortingLayerSettings::GetInstance()->GetLayerOrder(layerId),
		orderInLayer,
		stableOrder,
		nextSubmissionOrder_++});
}

void SpriteRenderer::Draw(ID3D12GraphicsCommandList* cmdList,
	PipelineService* psoService,
	RenderTargetType renderTarget) {
	if(sprites_.empty()) return;

	sprites_.erase(
		std::remove_if(sprites_.begin(), sprites_.end(), [renderTarget](const DrawEntry& entry) {
			return entry.sprite->GetTargetRt() != renderTarget;
		}),
		sprites_.end());

	if(sprites_.empty()) {
		Clear();
		return;
	}

	std::sort(sprites_.begin(), sprites_.end(), [](const DrawEntry& lhs, const DrawEntry& rhs) {
		if(lhs.sortingLayerOrder != rhs.sortingLayerOrder) return lhs.sortingLayerOrder < rhs.sortingLayerOrder;
		if(lhs.orderInLayer != rhs.orderInLayer) return lhs.orderInLayer < rhs.orderInLayer;
		if(lhs.stableOrder != rhs.stableOrder) return lhs.stableOrder < rhs.stableOrder;
		return lhs.submissionOrder < rhs.submissionOrder;
	});

	auto desc = PipelinePresets::MakeObject2D();
	psoService->SetCommand(desc, cmdList);
	for(const DrawEntry& entry : sprites_) entry.sprite->Draw(cmdList);
	Clear();
}

void SpriteRenderer::Clear() {
	sprites_.clear();
	nextSubmissionOrder_ = 0;
}
