#pragma once

#include <Engine/Assets/Font/FontTypes.h>

namespace CalyxEngine {
	class IGlyphProvider {
	public:
		virtual ~IGlyphProvider() = default;
		[[nodiscard]] virtual const GlyphInfo* GetGlyph(FontHandle font, char32_t codePoint, uint32_t pixelSize) = 0;
		[[nodiscard]] virtual FontLineMetrics GetLineMetrics(FontHandle font, uint32_t pixelSize) = 0;
	};
}
