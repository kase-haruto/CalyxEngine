#pragma once

#include <Engine/Foundation/Utility/Guid/Guid.h>

#include <string>

namespace CalyxEngine {
	struct DialogueLine {
		Guid guid_{};
		std::string speaker_;
		std::string text_;
		float charactersPerSecond_ = 30.0f;
		bool autoAdvance_ = false;
		float autoAdvanceDelay_ = 0.0f;
	};
}
