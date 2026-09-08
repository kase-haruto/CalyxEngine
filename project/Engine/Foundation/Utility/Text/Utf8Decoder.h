#pragma once

#include <Engine/Foundation/Export/CalyxAPI.h>

#include <string_view>
#include <vector>

namespace CalyxEngine {

	/*-----------------------------------------------------------------------------------------
	 * Utf8Decoder
	 * - UTF-8文字列をUnicodeコードポイント列へ変換する
	 * - 不正な入力はU+FFFDへ置換する
	 *---------------------------------------------------------------------------------------*/
	class Utf8Decoder {
	public:
		[[nodiscard]] CALYX_API static std::vector<char32_t> Decode(std::u8string_view text);
	};

} // namespace CalyxEngine
