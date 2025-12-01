#include "RailProgressBossSpawnService.h"

// engine
#include <Engine/Scene/Context/SceneContext.h>

void RailProgressBossSpawnService::BossSpawnByRailProgress() {
	auto spawner = wBossSpawner_.lock();
	auto railCamera = wRailCamera_.lock();
	auto player = wPlayer_.lock();
	if (!spawner || !railCamera || !player) return;

	if (spawner->WasSpawned()) return; // すでに出ていれば何もしない

	const float railProgress = railCamera->GetProgress();
	if (railProgress >= 0.8f) {
		spawner->SetPlayerTransform(player.get());
		spawner->Spawn();
	}
}

void RailProgressBossSpawnService::OnSceneLoaded(SceneContext& ctx) {
	wBossSpawner_ = ctx.FindFirst<BossSpawner>();
	wRailCamera_ = ctx.FindFirst<RailCamera>();
	wPlayer_ = ctx.FindFirst<Player>();
}
