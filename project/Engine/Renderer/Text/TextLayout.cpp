#include "TextLayout.h"

#include <Engine/Renderer/Text/IGlyphProvider.h>
#include <algorithm>
#include <cmath>

namespace CalyxEngine {
	TextLayoutResult TextLayout::Layout(
		FontHandle font, std::span<const char32_t> codePoints,
		const Vector2& origin, const TextStyle& style, IGlyphProvider& glyphProvider) const {
		TextLayoutResult result{};
		result.glyphs_.reserve(codePoints.size());
		const uint32_t pixelSize = (std::max)(1u, static_cast<uint32_t>(std::lround(style.fontSize_)));
		const FontLineMetrics metrics = glyphProvider.GetLineMetrics(font, pixelSize);
		const float lineHeight = (std::max)(1.0f, metrics.lineHeight_) * (std::max)(0.0f, style.lineSpacing_);
		result.baseline_ = origin.y + metrics.ascender_;

		float penX = origin.x;
		float baseline = result.baseline_;
		float maxX = origin.x;
		uint32_t lineCount = 1;
		for(uint32_t index = 0; index < codePoints.size(); ++index) {
			const char32_t codePoint = codePoints[index];
			if(codePoint == U'\r') continue;
			if(codePoint == U'\n') {
				maxX = (std::max)(maxX, penX);
				penX = origin.x;
				baseline += lineHeight;
				++lineCount;
				continue;
			}

			const GlyphInfo* glyph = glyphProvider.GetGlyph(font, codePoint, pixelSize);
			if(!glyph) continue;
			if(glyph->size_.x > 0.0f && glyph->size_.y > 0.0f) {
				result.glyphs_.push_back({*glyph,
					{penX + glyph->bearing_.x, baseline - glyph->bearing_.y}, index});
			}
			penX += glyph->advance_ + style.letterSpacing_;
		}
		maxX = (std::max)(maxX, penX);
		result.size_ = {(std::max)(0.0f, maxX - origin.x), lineHeight * lineCount};
		return result;
	}
}
