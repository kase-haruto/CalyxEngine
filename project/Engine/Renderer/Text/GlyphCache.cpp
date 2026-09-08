#include "GlyphCache.h"

#include <Engine/Assets/Font/FontRepository.h>
#include <Engine/Foundation/Log/EngineLogger.h>
#include <Engine/Renderer/Text/GlyphAtlas.h>

namespace CalyxEngine {
	size_t GlyphCache::KeyHash::operator()(const Key& key) const noexcept {
		size_t result = key.font_;
		result = result * 16777619u ^ key.codePoint_;
		return result * 16777619u ^ key.pixelSize_;
	}

	const GlyphInfo* GlyphCache::GetGlyph(FontHandle font, char32_t codePoint, uint32_t pixelSize) {
		Key key{font.value_, static_cast<uint32_t>(codePoint), pixelSize};
		if(const auto found = glyphs_.find(key); found != glyphs_.end()) return &found->second;
		if(const GlyphInfo* glyph = CreateGlyph(key)) return glyph;
		for(const char32_t fallback : {U'\uFFFD', U'?'}) {
			if(codePoint == fallback) continue;
			Key fallbackKey{font.value_, static_cast<uint32_t>(fallback), pixelSize};
			const GlyphInfo* fallbackGlyph = nullptr;
			if(const auto found = glyphs_.find(fallbackKey); found != glyphs_.end()) fallbackGlyph = &found->second;
			else fallbackGlyph = CreateGlyph(fallbackKey);
			if(fallbackGlyph) {
				// 存在しないCodePointにもReplacement Glyphを記録し、毎フレームの再Rasterizeを防ぐ。
				const GlyphInfo replacement = *fallbackGlyph;
				return &glyphs_.emplace(key, replacement).first->second;
			}
		}
		EngineLogger::GetInstance().Add(LogLevel::Warning, LogCategory::Asset,
			"Glyph and replacement glyph are unavailable.", "GlyphCache");
		return nullptr;
	}

	const GlyphInfo* GlyphCache::CreateGlyph(const Key& key) {
		auto bitmap = fonts_.Rasterize(FontHandle{key.font_}, static_cast<char32_t>(key.codePoint_), key.pixelSize_);
		if(!bitmap) return nullptr;
		GlyphInfo info{};
		if(!atlas_.Insert(*bitmap, info)) return nullptr;
		return &glyphs_.emplace(key, info).first->second;
	}

	FontLineMetrics GlyphCache::GetLineMetrics(FontHandle font, uint32_t pixelSize) {
		return fonts_.GetLineMetrics(font, pixelSize).value_or(
			FontLineMetrics{static_cast<float>(pixelSize), 0.0f, static_cast<float>(pixelSize)});
	}
}
