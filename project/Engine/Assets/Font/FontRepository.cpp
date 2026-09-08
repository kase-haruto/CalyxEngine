#include "FontRepository.h"

#include <CalyxEngine/Project.h>
#include <Engine/Assets/Font/FontAsset.h>
#include <Engine/Assets/Font/FontLoader.h>
#include <Engine/Assets/Font/FreeTypeBitmapRasterizer.h>
#include <Engine/Assets/Font/FreeTypeLibrary.h>
#include <Engine/Foundation/Log/EngineLogger.h>

namespace CalyxEngine {

	FontRepository::FontRepository()
		: library_(std::make_unique<FreeTypeLibrary>()),
		  loader_(std::make_unique<FontLoader>(*library_)),
		  rasterizer_(std::make_unique<FreeTypeBitmapRasterizer>()) {}

	FontRepository::~FontRepository() = default;

	FontHandle FontRepository::Load(const std::string& path) {
		const auto resolved = Calyx::ResolveAssetPath(path);
		const std::string key = resolved.lexically_normal().generic_string();
		if(const auto found = pathCache_.find(key); found != pathCache_.end()) return found->second;

		auto asset = loader_->Load(resolved);
		if(!asset) return {};

		const FontHandle handle{nextHandle_++};
		fonts_.emplace(handle.value_, std::move(asset));
		pathCache_.emplace(key, handle);
		EngineLogger::GetInstance().Add(LogLevel::Info, LogCategory::Asset,
			"Font loaded: " + key, "FontRepository");
		return handle;
	}

	const FontAsset* FontRepository::Find(FontHandle handle) const {
		const auto found = fonts_.find(handle.value_);
		return found != fonts_.end() ? found->second.get() : nullptr;
	}

	std::optional<RasterizedGlyph> FontRepository::Rasterize(
		FontHandle font, char32_t codePoint, uint32_t pixelSize) const {
		const FontAsset* asset = Find(font);
		return asset ? rasterizer_->Rasterize(*asset, codePoint, pixelSize) : std::nullopt;
	}

	std::optional<FontLineMetrics> FontRepository::GetLineMetrics(FontHandle font, uint32_t pixelSize) const {
		const FontAsset* asset = Find(font);
		return asset ? rasterizer_->GetLineMetrics(*asset, pixelSize) : std::nullopt;
	}
}
