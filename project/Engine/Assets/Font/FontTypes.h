#pragma once

#include <Engine/Foundation/Math/Vector2.h>

#include <cstdint>
#include <limits>
#include <vector>

namespace CalyxEngine {

	struct FontHandle {
		uint32_t value_ = 0;

		[[nodiscard]] bool IsValid() const noexcept { return value_ != 0; }
		explicit operator bool() const noexcept { return IsValid(); }
		bool operator==(const FontHandle&) const = default;
	};

	struct RasterizedGlyph {
		uint32_t width_ = 0;
		uint32_t height_ = 0;
		int32_t bearingX_ = 0;
		int32_t bearingY_ = 0;
		float advance_ = 0.0f;
		std::vector<uint8_t> pixels_;
	};

	struct FontLineMetrics {
		float ascender_ = 0.0f;
		float descender_ = 0.0f;
		float lineHeight_ = 0.0f;
	};

	struct GlyphInfo {
		Vector2 uvMin_{};
		Vector2 uvMax_{};
		Vector2 size_{};
		Vector2 bearing_{};
		float advance_ = 0.0f;
		uint32_t atlasPage_ = (std::numeric_limits<uint32_t>::max)();
	};

} // namespace CalyxEngine
