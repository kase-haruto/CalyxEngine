#include "EnemyInstaller.h"
#include <Game/Battle/Shooting/ShootingController/EnemyShootingController.h>
#include <Game/3dObject/Actor/Bullet/Container/EnemyBulletContainer.h>
#include <Engine/Scene/Utility/SceneUtility.h>

std::shared_ptr<Enemy> EnemyInstaller::InstallEnemy(){
	std::shared_ptr<Enemy> enemy = SceneAPI::Instantiate<Enemy>("ghost.obj", "enemy");

	std::unique_ptr<EnemyBulletContainer> enemyBulletContainer
			= std::make_unique<EnemyBulletContainer>("enemyBullerContainer");

	std::unique_ptr<EnemyShootingController> shootingController
			= std::make_unique<EnemyShootingController>(std::move(enemyBulletContainer));

	enemy->SetShootingController(std::move(shootingController));

	return enemy;
}
