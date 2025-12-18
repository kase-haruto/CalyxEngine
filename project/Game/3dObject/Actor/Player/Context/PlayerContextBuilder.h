#pragma once
#include "PlayerContext.h"

class Player;

class PlayerContextBuilder {
public:
	explicit PlayerContextBuilder(Player& player);

	PlayerActionContext BuildAction();
	PlayerStateContext  BuildState();

private:
	Player& player_;
};