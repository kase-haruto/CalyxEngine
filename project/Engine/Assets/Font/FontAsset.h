#pragma once

#include <Engine/Assets/Font/FontTypes.h>

#include <filesystem>

namespace CalyxEngine {

	/*-----------------------------------------------------------------------------------------
	 * FontAsset
	 * - 読み込まれたFont Faceとファイル情報を所有する
	 * - Glyph配置、Text Layout、GPU描画は担当しない
	 *---------------------------------------------------------------------------------------*/
	class FontAsset {
	public:
		FontAsset(std::filesystem::path path, void* face);
		~FontAsset();
		FontAsset(const FontAsset&) = delete;
		FontAsset& operator=(const FontAsset&) = delete;

		[[nodiscard]] const std::filesystem::path& GetPath() const noexcept { return path_; }
		[[nodiscard]] void* GetNativeFace() const noexcept { return face_; }

	private:
		std::filesystem::path path_;
		void* face_ = nullptr; //< FT_Face。FreeType型を公開Headerへ漏らさない
	};

} // namespace CalyxEngine
