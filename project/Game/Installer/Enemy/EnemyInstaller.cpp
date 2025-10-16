#include "EnemyInstaller.h"
#include <Game/Battle/Shooting/ShootingController/EnemyShootingController.h>
#include <Game/3dObject/Actor/Bullet/Container/EnemyBulletContainer.h>
#include <Engine/Scene/Utility/SceneUtility.h>

std::shared_ptr<Enemy> EnemyInstaller::InstallEnemy(EnemyBulletContainer* bulletContainer){
	std::shared_ptr<Enemy> enemy = SceneAPI::Instantiate<Enemy>("ghost.obj", "enemy");

	// 敵の共通の弾Containerを使用する
	std::unique_ptr<EnemyShootingController> shootingController
			= std::make_unique<EnemyShootingController>(bulletContainer);

	enemy->SetShootingController(std::move(shootingController));

	return enemy;
}