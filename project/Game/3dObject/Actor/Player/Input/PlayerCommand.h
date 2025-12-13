#pragma once
#include <variant>
#include <Engine/Foundation/Math/Vector3.h>

enum class PlayerCommandType {
	Move,
	MoveReticle,
	Shoot,
	Dodge,
	LockOn,
	ClearLockOn
};

struct CmdMove {
	Vector3 delta;
};

struct PlayerCommand {
	PlayerCommandType type;
	std::variant<std::monostate, CmdMove> value;
};