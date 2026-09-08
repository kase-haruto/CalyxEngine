#pragma once

#include <Engine/Assets/Font/FontTypes.h>
#include <Engine/Foundation/Math/Vector2.h>
#include <Engine/Renderer/Text/TextStyle.h>

#include <span>
#include <vector>

namespace CalyxEngine {
	class IGlyphProvider;

	struct PositionedGlyph {
		GlyphInfo glyph_{};
		Vector2 position_{};
		uint32_t sourceIndex_ = 0;
	};

	struct TextLayoutResult {
		std::vector<PositionedGlyph> glyphs_;
		Vector2 size_{};
		float baseline_ = 0.0f;
	};

	/*-----------------------------------------------------------------------------------------
	 * TextLayout
	 * - CodePoint列とGlyph Metricsから描画位置を計算する
	 * - DirectX 12とGPU描画には依存しない
	 *---------------------------------------------------------------------------------------*/
	class TextLayout {
	public:
		[[nodiscard]] TextLayoutResult Layout(
			FontHandle font, std::span<const char32_t> codePoints,
			const Vector2& origin, const TextStyle& style, IGlyphProvider& glyphProvider) const;
	};
}
