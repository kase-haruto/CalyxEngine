#include "BossInstaller.h"

#include <Game/3dObject/Actor/Bullet/Container/BossBulletContainer.h>
#include <Game/Battle/Shooting/ShootingController/BossShootingController.h>
#include <Engine/Scene/Utility/SceneUtility.h>

std::shared_ptr<Boss> BossInstaller::InstallBoss() {
	std::shared_ptr<Boss> boss = SceneAPI::Instantiate<Boss>("boss.gltf", "boss");

	std::unique_ptr<BossBulletContainer> bossBulletContainer
		= std::make_unique<BossBulletContainer>("BossBullerContainer");

	std::unique_ptr<BossShootingController> shootingController
		= std::make_unique<BossShootingController>(std::move(bossBulletContainer));

	boss->SetShootingController(std::move(shootingController));

	return boss;
}
