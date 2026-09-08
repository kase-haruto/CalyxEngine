#include "FontLoader.h"

#include <Engine/Assets/Font/FontAsset.h>
#include <Engine/Assets/Font/FreeTypeLibrary.h>
#include <Engine/Foundation/Log/EngineLogger.h>
#include <ft2build.h>
#include FT_FREETYPE_H

namespace CalyxEngine {

	std::unique_ptr<FontAsset> FontLoader::Load(const std::filesystem::path& path) const {
		if(!library_.IsValid() || !std::filesystem::is_regular_file(path)) {
			EngineLogger::GetInstance().Add(LogLevel::Error, LogCategory::Asset,
				"Font file not found: " + path.generic_string(), "FontLoader");
			return nullptr;
		}

		FT_Face face = nullptr;
		const auto nativePath = path.string();
		if(FT_New_Face(static_cast<FT_Library>(library_.GetNativeLibrary()), nativePath.c_str(), 0, &face) != 0) {
			EngineLogger::GetInstance().Add(LogLevel::Error, LogCategory::Asset,
				"FreeType failed to create face: " + path.generic_string(), "FontLoader");
			return nullptr;
		}
		return std::make_unique<FontAsset>(path, face);
	}
}
