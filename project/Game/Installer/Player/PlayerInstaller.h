#pragma once

#include <Game/3dObject/Actor/Player/Player.h>
#include <memory>

class PlayerInstaller{
public:
	PlayerInstaller() = default;
	std::shared_ptr<Player> InstallPlayer(const std::shared_ptr<Player>& player);

};

