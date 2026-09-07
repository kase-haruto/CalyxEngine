#include "Utf8Decoder.h"

namespace CalyxEngine {

	std::vector<char32_t> Utf8Decoder::Decode(std::u8string_view text) {
		std::vector<char32_t> result;
		result.reserve(text.size());

		for(size_t i = 0; i < text.size();) {
			const auto first = static_cast<unsigned char>(text[i]);
			char32_t codePoint = 0;
			size_t length = 0;
			char32_t minimum = 0;

			if(first < 0x80) {
				codePoint = first;
				length = 1;
			} else if((first & 0xE0) == 0xC0) {
				codePoint = first & 0x1F;
				length = 2;
				minimum = 0x80;
			} else if((first & 0xF0) == 0xE0) {
				codePoint = first & 0x0F;
				length = 3;
				minimum = 0x800;
			} else if((first & 0xF8) == 0xF0) {
				codePoint = first & 0x07;
				length = 4;
				minimum = 0x10000;
			} else {
				result.push_back(U'\uFFFD');
				++i;
				continue;
			}

			if(i + length > text.size()) {
				result.push_back(U'\uFFFD');
				break;
			}

			bool valid = true;
			for(size_t offset = 1; offset < length; ++offset) {
				const auto continuation = static_cast<unsigned char>(text[i + offset]);
				if((continuation & 0xC0) != 0x80) {
					valid = false;
					break;
				}
				codePoint = (codePoint << 6) | (continuation & 0x3F);
			}

			if(!valid || codePoint < minimum || codePoint > 0x10FFFF ||
			   (codePoint >= 0xD800 && codePoint <= 0xDFFF)) {
				result.push_back(U'\uFFFD');
				++i;
				continue;
			}

			result.push_back(codePoint);
			i += length;
		}
		return result;
	}

} // namespace CalyxEngine
