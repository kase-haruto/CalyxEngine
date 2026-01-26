#pragma once
#include <Engine/Foundation/Math/Vector3.h>
#include <variant>


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

struct CmdDodge {
	CalyxMath::Vector3 dir;
};

struct PlayerCommand {
	PlayerCommandType								type;
	std::variant<std::monostate, CmdMove, CmdDodge> value;
};