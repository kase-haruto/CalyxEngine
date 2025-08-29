#pragma once

#include <Game/3dObject/Actor/Enemy/Enemy.h>

#include <memory>

class EnemyInstaller{
public:
	EnemyInstaller() = default;
	std::shared_ptr<Enemy> InstallEnemy();
};
