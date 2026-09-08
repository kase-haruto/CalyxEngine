#pragma once

#include <filesystem>
#include <memory>

namespace CalyxEngine {
	class FontAsset;
	class FreeTypeLibrary;

	/*-----------------------------------------------------------------------------------------
	 * FontLoader
	 * - .ttf/.otfからFontAssetを生成する
	 * - FreeType固有のFace生成処理をRendererから隔離する
	 *---------------------------------------------------------------------------------------*/
	class FontLoader {
	public:
		explicit FontLoader(FreeTypeLibrary& library) : library_(library) {}
		[[nodiscard]] std::unique_ptr<FontAsset> Load(const std::filesystem::path& path) const;

	private:
		FreeTypeLibrary& library_;
	};
}
