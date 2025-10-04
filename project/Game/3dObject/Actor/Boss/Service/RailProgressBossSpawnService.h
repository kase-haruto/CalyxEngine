#pragma once

#include <Game/3dObject/Actor/Boss/Spawner/BossSpawner.h>
#include <Game/3d/GameCamera/RailCamera.h>
#include <Game/3dObject/Actor/Player/Player.h>

class RailProgressBossSpawnService {
public:
	//===================================================================*/
	//						public methods
	//===================================================================*/
	RailProgressBossSpawnService() = default;
	~RailProgressBossSpawnService() = default;

	void BossSpawnByRailProgress();
	void OnSceneLoaded(class SceneContext& ctx);

private:
	//===================================================================*/
	//						private methods
	//===================================================================*/
	std::weak_ptr<BossSpawner> wBossSpawner_;
	std::weak_ptr<RailCamera> wRailCamera_;
	std::weak_ptr<Player> wPlayer_;
};