#pragma once

#include <Engine/Renderer/Text/IGlyphProvider.h>
#include <unordered_map>

namespace CalyxEngine {
	class FontRepository;
	class GlyphAtlas;

	/*-----------------------------------------------------------------------------------------
	 * GlyphCache
	 * - Font、Pixel Size、Unicode CodePoint単位でGlyph情報をキャッシュする
	 * - 未生成GlyphだけをRasterizeしてAtlasへ登録する
	 *---------------------------------------------------------------------------------------*/
	class GlyphCache final : public IGlyphProvider {
	public:
		GlyphCache(FontRepository& fonts, GlyphAtlas& atlas) : fonts_(fonts), atlas_(atlas) {}
		[[nodiscard]] const GlyphInfo* GetGlyph(FontHandle font, char32_t codePoint, uint32_t pixelSize) override;
		[[nodiscard]] FontLineMetrics GetLineMetrics(FontHandle font, uint32_t pixelSize) override;
		[[nodiscard]] size_t GetGlyphCount() const noexcept { return glyphs_.size(); }

	private:
		struct Key {
			uint32_t font_ = 0;
			uint32_t codePoint_ = 0;
			uint32_t pixelSize_ = 0;
			bool operator==(const Key&) const = default;
		};
		struct KeyHash { size_t operator()(const Key& key) const noexcept; };
		const GlyphInfo* CreateGlyph(const Key& key);

		FontRepository& fonts_;
		GlyphAtlas& atlas_;
		std::unordered_map<Key, GlyphInfo, KeyHash> glyphs_;
	};
}
