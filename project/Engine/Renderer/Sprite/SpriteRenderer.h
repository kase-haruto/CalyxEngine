#pragma once

#include <Engine/Graphics/RenderTarget/Detail/RenderTargetDetail.h>
#include <Engine/Renderer/Sprite/SortingLayerSettings.h>

#include <cstdint>
#include <d3d12.h>
#include <vector>

class Sprite;

/**
 * @brief SpriteRendererの機能を提供するクラスです。
 */
class SpriteRenderer {
public:
	// Compatibility entry point for sprites without explicit sorting settings.
	void Register(Sprite* sprite);
	void Register(Sprite* sprite, SortingLayerId layerId, int32_t orderInLayer, uint64_t stableOrder);

	void Draw(ID3D12GraphicsCommandList* cmdList,
		class PipelineService* psoService,
		RenderTargetType renderTargetType);
	void Clear();

private:
	/**
	 * @brief DrawEntryに関するデータを保持する構造体です。
	 */
	struct DrawEntry {
		Sprite* sprite = nullptr;
		int32_t sortingLayerOrder = 0;
		int32_t orderInLayer = 0;
		uint64_t stableOrder = 0;
		uint64_t submissionOrder = 0;
	};

	std::vector<DrawEntry> sprites_;
	uint64_t nextSubmissionOrder_ = 0;
};
