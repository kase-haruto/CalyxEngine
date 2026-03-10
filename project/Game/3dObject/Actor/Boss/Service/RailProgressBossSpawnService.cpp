#include "RailProgressBossSpawnService.h"

// engine
#include "Game/3dObject/Actor/Player/DangerSense/PlayerDangerSense.h"

#include <Engine/Scene/Context/SceneContext.h>

void RailProgressBossSpawnService::BossSpawnByRailProgress() {
	auto spawner = wBossSpawner_.lock();
	auto player = wPlayer_.lock();
	if (!spawner || !player) return;

	// スポーンされていたらプレイヤーの危機察知にボスの弾コンテナを登録
	if (spawner->WasSpawned()) {
		// ボスの弾コンテナをプレイヤーの危機察知に登録
		auto boss = spawner->GetBoss().lock();
		if (boss) {
			auto bossBulletContainer = boss->GetBulletContainer();
			if (bossBulletContainer) {
				player->GetDangerSense()->AddBulletContainer(bossBulletContainer);
			}
		}
	}
}

void RailProgressBossSpawnService::OnSceneLoaded(SceneContext& ctx) {
	wBossSpawner_ = ctx.FindFirst<BossSpawner>();
	wPlayer_ = ctx.FindFirst<Player>();
}