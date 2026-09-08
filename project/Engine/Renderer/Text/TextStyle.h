#pragma once

#include <Engine/Foundation/Math/Vector4.h>

namespace CalyxEngine {
	struct TextStyle {
		float fontSize_ = 32.0f;
		Vector4 color_{1.0f, 1.0f, 1.0f, 1.0f};
		float letterSpacing_ = 0.0f;
		float lineSpacing_ = 1.0f;
		float maxWidth_ = 0.0f; //< 0以下の場合は自動折り返しを行わない
		bool wordWrap_ = false;
	};
}
