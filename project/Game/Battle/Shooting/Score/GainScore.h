#pragma once

// engine
#include <Engine/Foundation/Utility/Guid/Guid.h>

// stl
#include <string>
#include <cstdint>

namespace ScoreReason {
	constexpr const char* EnemyKill = "enemyKill";
}

struct GainScore{
	int32_t amount;
	Guid id;
	std::string reason;
	std::vector<std::string> tag;
};