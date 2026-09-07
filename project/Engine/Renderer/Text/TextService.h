#pragma once

#include <Engine/Assets/Font/FontTypes.h>
#include <Engine/Foundation/Export/CalyxAPI.h>
#include <Engine/Foundation/Math/Vector2.h>
#include <Engine/Renderer/Text/TextStyle.h>

#include <d3d12.h>
#include <memory>
#include <string>
#include <string_view>

class PipelineService;

namespace CalyxEngine {
	class GlyphAtlas;
	class GlyphCache;
	class TextLayout;
	class TextRenderer;

	struct TextDebugStats {
		size_t fontCount_ = 0;
		size_t glyphCount_ = 0;
		size_t atlasPageCount_ = 0;
		uint32_t atlasPageSize_ = 0;
		float atlasUsage_ = 0.0f;
	};

	/*-----------------------------------------------------------------------------------------
	 * TextService
	 * - Game、Runtime、Editor向けの汎用Text描画Facade
	 * - 各処理はFont、Cache、Layout、Rendererへ委譲する
	 *---------------------------------------------------------------------------------------*/
	class CALYX_API TextService {
	public:
		static TextService* GetInstance();
		bool Initialize(ID3D12Device* device);
		void Finalize();
		void BeginFrame();

		[[nodiscard]] FontHandle LoadFont(const std::string& path);
		void Draw(FontHandle font, std::u8string_view text, const Vector2& position, const TextStyle& style = {});
		void Draw(FontHandle font, std::u8string_view text, const Vector2& position,
			const TextStyle& style, size_t visibleGlyphCount);
		void Render(ID3D12GraphicsCommandList* commandList, PipelineService* pipelines,
			uint32_t viewportWidth, uint32_t viewportHeight);
		[[nodiscard]] TextDebugStats GetDebugStats() const;

	private:
		TextService();
		~TextService();
		struct Impl;
		std::unique_ptr<Impl> impl_;
	};
}
