#include "EnemyRuntimeBindingService.h"
#include <Game/3dObject/Actor/Enemy/Directory/EnemyDirectory.h>

// engine
#include <Engine/Scene/Context/SceneContext.h>
#include <Engine/objects/3D/Actor/Library/SceneObjectLibrary.h>

// game
#include "Game/3dObject/Actor/Player/DangerSense/PlayerDangerSense.h"

#include <Game/3dObject/Actor/Player/Player.h>
#include <Game/3dObject/Actor/Enemy/Spawner/EnemySpawner.h>
#include <Game/Installer/Player/PlayerInstaller.h>

void EnemyRuntimeBindingService::OnSceneLoaded(SceneContext& ctx,EnemyBulletContainer* bulletContainer) {
	dir_ = std::make_shared<EnemyDirectory>();
	dir_->Clear();

	// プレイヤー構築
	auto player = ctx.FindFirst<Player>();
	PlayerInstaller{}.InstallPlayer(player);
	wPlayer_ = player;
	player->AttachDangerSenseSource(dir_.get());

	// 弾コンテナ情報
	bulletContainer_ = bulletContainer;
	//プレイヤーの危機察知用に渡す
	player->GetDangerSense()->SetEnemyBulletContainer(bulletContainer);

	// スポナー
	WireAllSpawners(ctx);
}

void EnemyRuntimeBindingService::Update(SceneContext& ctx,float) {
	auto player = wPlayer_.lock();
	if(!player) {
		player   = ctx.FindFirst<Player>();
		wPlayer_ = player;
		if(player) WireAllSpawners(ctx);
	}

	// ランタイム中にスポナーが増減したら再配線
	auto*  lib       = ctx.GetObjectLibrary();
	size_t currCount = 0;
	for(auto& obj : lib->GetAllObjectsShared()) if(std::dynamic_pointer_cast<EnemySpawner>(obj)) ++currCount;
	if(currCount != lastSpawnerCount_) { WireAllSpawners(ctx); }

	if(player && dir_) { player->SetEnemyList(dir_->SnapshotAlive()); }
}

void EnemyRuntimeBindingService::OnSceneCleared(SceneContext&) {
	if(dir_) dir_->Clear();
	dir_.reset();
	wPlayer_.reset();
	lastSpawnerCount_ = 0;
}

void EnemyRuntimeBindingService::WireAllSpawners(SceneContext& ctx) {
	auto   player = wPlayer_.lock();
	auto*  lib    = ctx.GetObjectLibrary();
	size_t wired  = 0;

	for(auto& obj : lib->GetAllObjectsShared()) {
		if(auto sp = std::dynamic_pointer_cast<EnemySpawner>(obj)) {

			// enemyでtargetように使用するためスポナーにtransformを渡してそれを参照
			if(player) sp->SetPlayerTransform(&player->GetWorldTransform());
			sp->SetBulletContainer(bulletContainer_);
			sp->SetDirectory(dir_.get());
			++wired;
		}
	}
	lastSpawnerCount_ = wired;
}