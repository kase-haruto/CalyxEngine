#include "RailProgressBossSpawnService.h"

// engine
#include <Engine/Scene/Context/SceneContext.h>

void RailProgressBossSpawnService::BossSpawnByRailProgress() {
	auto spawner = wBossSpawner_.lock();
	auto railCamera = wRailCamera_.lock();

	float railProgress = railCamera->GetProgress();

	//Railの進捗が8割まで来たら、ボスをスポーンさせる。
	if (railProgress >= 0.8f) {
		spawner->Spawn();
	}
}

void RailProgressBossSpawnService::OnSceneLoaded(SceneContext& ctx) {
	wBossSpawner_ = ctx.FindFirst<BossSpawner>();
	wRailCamera_ = ctx.FindFirst<RailCamera>();
}
