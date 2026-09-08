#pragma once

#include <Engine/Assets/Font/FontTypes.h>

#include <optional>

namespace CalyxEngine {
	class FontAsset;

	/*-----------------------------------------------------------------------------------------
	 * FreeTypeBitmapRasterizer
	 * - Font Faceから8bit Bitmap GlyphとMetricsを生成する
	 * - Atlas配置やGPU転送は担当しない
	 *---------------------------------------------------------------------------------------*/
	class FreeTypeBitmapRasterizer {
	public:
		[[nodiscard]] std::optional<RasterizedGlyph> Rasterize(
			const FontAsset& font, char32_t codePoint, uint32_t pixelSize) const;
		[[nodiscard]] std::optional<FontLineMetrics> GetLineMetrics(
			const FontAsset& font, uint32_t pixelSize) const;
	};
}
