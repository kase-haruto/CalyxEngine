#include "PlayerInstaller.h"
#include <Game/3dObject/Actor/Bullet/Container/PlayerBulletContainer.h>

std::shared_ptr<Player> PlayerInstaller::InstallPlayer(const std::shared_ptr<Player>& player){

	// BulletContainer を作る
	auto bulletContainer = std::make_unique<PlayerBulletContainer>("playerBulletContainer");

	// ShootingController を bulletContainer を 所有
	auto shooting = std::make_unique<PlayerShootingController>();

	// ShootingController に container の所有権を渡す
	shooting->SetBulletContainer(std::move(bulletContainer));

	shooting->Initialize();

	// Player に ShootingController をセット
	player->SetShootingController(std::move(shooting));

	return player;
}