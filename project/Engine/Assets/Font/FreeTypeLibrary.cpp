#include "FreeTypeLibrary.h"

#include <Engine/Foundation/Log/EngineLogger.h>
#include <ft2build.h>
#include FT_FREETYPE_H

namespace CalyxEngine {

	FreeTypeLibrary::FreeTypeLibrary() {
		FT_Library library = nullptr;
		if(FT_Init_FreeType(&library) != 0) {
			EngineLogger::GetInstance().Add(LogLevel::Error, LogCategory::Asset,
				"FreeType initialization failed.", "FreeTypeLibrary");
			return;
		}
		library_ = library;
	}

	FreeTypeLibrary::~FreeTypeLibrary() {
		if(library_) FT_Done_FreeType(static_cast<FT_Library>(library_));
	}

} // namespace CalyxEngine
