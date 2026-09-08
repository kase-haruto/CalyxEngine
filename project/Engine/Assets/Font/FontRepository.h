#pragma once

#include <Engine/Assets/Font/FontTypes.h>
#include <Engine/Foundation/Export/CalyxAPI.h>

#include <memory>
#include <optional>
#include <string>
#include <unordered_map>

namespace CalyxEngine {
	class FontAsset;
	class FontLoader;
	class FreeTypeBitmapRasterizer;
	class FreeTypeLibrary;

	/*-----------------------------------------------------------------------------------------
	 * FontRepository
	 * - FontAssetをパス単位でロード、キャッシュする
	 * - Layout、Atlas、GPU描画は担当しない
	 *---------------------------------------------------------------------------------------*/
	class CALYX_API FontRepository {
	public:
		FontRepository();
		~FontRepository();
		FontRepository(const FontRepository&) = delete;
		FontRepository& operator=(const FontRepository&) = delete;

		[[nodiscard]] FontHandle Load(const std::string& path);
		[[nodiscard]] std::optional<RasterizedGlyph> Rasterize(FontHandle font, char32_t codePoint, uint32_t pixelSize) const;
		[[nodiscard]] std::optional<FontLineMetrics> GetLineMetrics(FontHandle font, uint32_t pixelSize) const;
		[[nodiscard]] size_t GetFontCount() const noexcept { return fonts_.size(); }

	private:
		[[nodiscard]] const FontAsset* Find(FontHandle handle) const;

		std::unique_ptr<FreeTypeLibrary> library_;
		std::unique_ptr<FontLoader> loader_;
		std::unique_ptr<FreeTypeBitmapRasterizer> rasterizer_;
		std::unordered_map<uint32_t, std::unique_ptr<FontAsset>> fonts_;
		std::unordered_map<std::string, FontHandle> pathCache_;
		uint32_t nextHandle_ = 1;
	};
}
