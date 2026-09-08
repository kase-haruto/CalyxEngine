#include "FreeTypeBitmapRasterizer.h"

#include <Engine/Assets/Font/FontAsset.h>
#include <ft2build.h>
#include FT_FREETYPE_H

#include <algorithm>
#include <cstring>

namespace CalyxEngine {

	std::optional<RasterizedGlyph> FreeTypeBitmapRasterizer::Rasterize(
		const FontAsset& font, char32_t codePoint, uint32_t pixelSize) const {
		auto face = static_cast<FT_Face>(font.GetNativeFace());
		if(!face || pixelSize == 0 || FT_Set_Pixel_Sizes(face, 0, pixelSize) != 0) return std::nullopt;
		const FT_UInt glyphIndex = FT_Get_Char_Index(face, static_cast<FT_ULong>(codePoint));
		if(glyphIndex == 0 || FT_Load_Glyph(face, glyphIndex, FT_LOAD_DEFAULT) != 0 ||
		   FT_Render_Glyph(face->glyph, FT_RENDER_MODE_NORMAL) != 0) return std::nullopt;

		const FT_Bitmap& bitmap = face->glyph->bitmap;
		RasterizedGlyph result{};
		result.width_ = bitmap.width;
		result.height_ = bitmap.rows;
		result.bearingX_ = face->glyph->bitmap_left;
		result.bearingY_ = face->glyph->bitmap_top;
		result.advance_ = static_cast<float>(face->glyph->advance.x) / 64.0f;
		result.pixels_.resize(static_cast<size_t>(result.width_) * result.height_);

		for(uint32_t y = 0; y < result.height_; ++y) {
			const int pitch = bitmap.pitch;
			const uint8_t* source = pitch >= 0
				? bitmap.buffer + static_cast<size_t>(y) * pitch
				: bitmap.buffer + static_cast<size_t>(result.height_ - 1 - y) * static_cast<size_t>(-pitch);
			std::memcpy(result.pixels_.data() + static_cast<size_t>(y) * result.width_, source, result.width_);
		}
		return result;
	}

	std::optional<FontLineMetrics> FreeTypeBitmapRasterizer::GetLineMetrics(
		const FontAsset& font, uint32_t pixelSize) const {
		auto face = static_cast<FT_Face>(font.GetNativeFace());
		if(!face || pixelSize == 0 || FT_Set_Pixel_Sizes(face, 0, pixelSize) != 0 || !face->size) return std::nullopt;
		return FontLineMetrics{
			static_cast<float>(face->size->metrics.ascender) / 64.0f,
			static_cast<float>(face->size->metrics.descender) / 64.0f,
			static_cast<float>(face->size->metrics.height) / 64.0f};
	}
}
