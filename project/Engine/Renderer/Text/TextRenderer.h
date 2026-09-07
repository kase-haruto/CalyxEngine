#pragma once

#include <Engine/Foundation/Math/Vector4.h>
#include <Engine/Renderer/Text/TextLayout.h>

#include <d3d12.h>
#include <limits>
#include <vector>
#include <wrl.h>

class PipelineService;

namespace CalyxEngine {
	class GlyphAtlas;

	struct TextDrawData {
		const TextLayoutResult* layout_ = nullptr;
		Vector4 color_{};
		size_t visibleGlyphCount_ = (std::numeric_limits<size_t>::max)();
	};

	/*-----------------------------------------------------------------------------------------
	 * TextRenderer
	 * - TextLayoutResultからGPU頂点を生成し、Atlas Page単位で描画する
	 * - Font読み込み、UTF-8解析、Glyph Rasterize、Layoutは担当しない
	 *---------------------------------------------------------------------------------------*/
	class TextRenderer {
	public:
		bool Initialize(ID3D12Device* device);
		void Finalize();
		void Draw(ID3D12GraphicsCommandList* commandList, PipelineService* pipelines,
			const GlyphAtlas& atlas, const std::vector<TextDrawData>& draws,
			uint32_t viewportWidth, uint32_t viewportHeight);

	private:
		struct Vertex { Vector2 position_{}; Vector2 uv_{}; Vector4 color_{}; };
		struct PageBatch { uint32_t page_ = 0; std::vector<Vertex> vertices_; };
		bool EnsureVertexCapacity(size_t vertexCount);

		ID3D12Device* device_ = nullptr;
		Microsoft::WRL::ComPtr<ID3D12Resource> vertexBuffer_;
		Vertex* mappedVertices_ = nullptr;
		size_t vertexCapacity_ = 0;
		std::vector<PageBatch> batches_;
	};
}
