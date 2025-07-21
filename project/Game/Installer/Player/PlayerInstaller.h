#pragma once

#include <Game/3dObject/Actor/Player/Player.h>
#include <memory>

class PlayerInstaller{
public:
	std::shared_ptr<Player> InstallPlayer();

};

