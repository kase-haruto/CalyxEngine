#pragma once
#include <variant>
#include <Engine/Foundation/Math/Vector3.h>

enum class PlayerCommandType {
	Move,
	Shoot,
	Dodge,
	LockOn,
	ClearLockOn
};

struct CmdMove {
	CalyxMath::Vector3 delta;
};

struct PlayerCommand {
	PlayerCommandType type;
	std::variant<std::monostate, CmdMove> value;
};