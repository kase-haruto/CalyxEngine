#pragma once
#include <vector>
#include "PlayerCommand.h"

class PlayerInput {
public:
	std::vector<PlayerCommand> CollectCommands(float dt);
};
