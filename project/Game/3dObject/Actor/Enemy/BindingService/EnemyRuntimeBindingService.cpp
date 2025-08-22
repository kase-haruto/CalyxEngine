#include "EnemyRuntimeBindingService.h"
#include <Game/3dObject/Actor/Enemy/Directory/EnemyDirectory.h>

// engine
#include <Engine/Scene/Context/SceneContext.h>
#include <Engine/objects/3D/Actor/Library/SceneObjectLibrary.h>

// game
#include <Game/3dObject/Actor/Player/Player.h>
#include <Game/3dObject/Actor/Enemy/Spawner/EnemySpawner.h>
#include <Game/Installer/Player/PlayerInstaller.h>

void EnemyRuntimeBindingService::OnSceneLoaded(SceneContext& ctx) {
	dir_ = std::make_shared<EnemyDirectory>();
	dir_->Clear();

	// プレイヤー構築
	auto player = ctx.FindFirst<Player>();
	PlayerInstaller{}.InstallPlayer(player);
	wPlayer_ = player;

	// 既存スポナーへ一括配線
	WireAllSpawners_(ctx);
}

void EnemyRuntimeBindingService::Update(SceneContext& ctx, float) {
	auto player = wPlayer_.lock();
	if (!player) {
		// プレイヤーがまだなら再取得して配線
		player = ctx.FindFirst<Player>();
		wPlayer_ = player;
		if (player) WireAllSpawners_(ctx);
	}

	// ランタイム中にスポナーが増減したら再配線
	auto* lib = ctx.GetObjectLibrary();
	size_t currCount = 0;
	for (auto& obj : lib->GetAllObjectsShared())
		if (std::dynamic_pointer_cast<EnemySpawner>(obj)) ++currCount;
	if (currCount != lastSpawnerCount_) {
		WireAllSpawners_(ctx);
	}

	// 毎フレーム：最新の敵リストを Player に供給
	if (player && dir_) {
		player->SetEnemyList(dir_->SnapshotAlive());
	}
}

void EnemyRuntimeBindingService::OnSceneCleared(SceneContext&) {
	if (dir_) dir_->Clear();
	dir_.reset();
	wPlayer_.reset();
	lastSpawnerCount_ = 0;
}

void EnemyRuntimeBindingService::WireAllSpawners_(SceneContext& ctx) {
	auto player = wPlayer_.lock();
	auto* lib = ctx.GetObjectLibrary();
	size_t wired = 0;
	for (auto& obj : lib->GetAllObjectsShared()) {
		if (auto sp = std::dynamic_pointer_cast<EnemySpawner>(obj)) {
			if (player) sp->SetPlayerTransform(&player->GetWorldTransform()); // 親は Player 本人
			sp->SetDirectory(dir_.get()); // ディレクトリを注入
			++wired;
		}
	}
	lastSpawnerCount_ = wired;
}
