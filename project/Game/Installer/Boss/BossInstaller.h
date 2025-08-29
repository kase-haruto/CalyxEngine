#pragma once

#include <Game/3dObject/Actor/Boss/Boss.h>
#include <memory>

class BossInstaller {
public:
	BossInstaller() = default;
	std::shared_ptr<Boss> InstallBoss();
};

