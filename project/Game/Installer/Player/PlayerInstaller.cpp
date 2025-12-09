#include "PlayerInstaller.h"
#include <Game/3dObject/Actor/Bullet/Container/PlayerBulletContainer.h>
#include <Game/Input/PlayerInput/PlayerInputHandler.h>

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

	// input
	auto input = std::make_unique<PlayerInputHandler>();
	player->SetInputHandler(std::move(input));

	return player;
}