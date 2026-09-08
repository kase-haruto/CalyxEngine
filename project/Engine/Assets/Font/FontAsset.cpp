#include "FontAsset.h"

#include <ft2build.h>
#include FT_FREETYPE_H

namespace CalyxEngine {

	FontAsset::FontAsset(std::filesystem::path path, void* face)
		: path_(std::move(path)), face_(face) {}

	FontAsset::~FontAsset() {
		if(face_) FT_Done_Face(static_cast<FT_Face>(face_));
	}

} // namespace CalyxEngine
