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
	// Spriteの所有権は呼び出し側に残し、当該フレームの描画情報だけをQueueへ登録する。
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

	// 複数Viewport向け登録から、現在のRenderTargetに属するSpriteだけを抽出する。
	sprites_.erase(
		std::remove_if(sprites_.begin(), sprites_.end(), [renderTarget](const DrawEntry& entry) {
			return entry.sprite->GetTargetRt() != renderTarget;
		}),
		sprites_.end());

	if(sprites_.empty()) {
		Clear();
		return;
	}

	// Layer、Layer内Order、安定Key、登録順の順で比較し、同値時も描画順を決定的にする。
	std::sort(sprites_.begin(), sprites_.end(), [](const DrawEntry& lhs, const DrawEntry& rhs) {
		if(lhs.sortingLayerOrder != rhs.sortingLayerOrder) return lhs.sortingLayerOrder < rhs.sortingLayerOrder;
		if(lhs.orderInLayer != rhs.orderInLayer) return lhs.orderInLayer < rhs.orderInLayer;
		if(lhs.stableOrder != rhs.stableOrder) return lhs.stableOrder < rhs.stableOrder;
		return lhs.submissionOrder < rhs.submissionOrder;
	});

	// 2D共通Pipelineを一度設定し、整列済みSpriteのDraw命令を順番に記録する。
	auto desc = PipelinePresets::MakeObject2D();
	psoService->SetCommand(desc, cmdList);
	for(const DrawEntry& entry : sprites_) entry.sprite->Draw(cmdList);
	Clear();
}

void SpriteRenderer::Clear() {
	// Queueはフレーム単位の非所有参照なので、描画後に必ず破棄してLifetime跨ぎを防ぐ。
	sprites_.clear();
	nextSubmissionOrder_ = 0;
}
