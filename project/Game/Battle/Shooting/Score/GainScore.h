#pragma once

// engine
#include <Engine/Foundation/Utility/Guid/Guid.h>
#include <Game/3dObject/Actor/Enemy/Details/EnemyKind.h>
// stl
#include <string>
#include <cstdint>

namespace ScoreReason {
	constexpr const char* EnemyKill = "enemyKill";
}

struct GainScore{
	int32_t amount;
	Guid id;
	EnemyKind enemyKind;
};