#include "PlayerInstaller.h"
#include <Game/3dObject/Actor/Bullet/Container/PlayerBulletContainer.h>
#include <Game/Input/PlayerInput/PlayerInputHandler.h>

std::shared_ptr<Player> PlayerInstaller::InstallPlayer(const std::shared_ptr<Player>& player){
	// shoot
	auto playerBulletContainer_ = std::make_unique<PlayerBulletContainer>("playerBulletContainer");
	auto playerShootingController_ = std::make_unique<PlayerShootingController>(playerBulletContainer_.get());
	playerShootingController_->SetBulletContainer(std::move(playerBulletContainer_));
	
	// input
	auto playerInput = std::make_unique<PlayerInputHandler>();

	player->SetShootingController(std::move(playerShootingController_));
	player->SetInputHandler(std::move(playerInput));

	return player;
}
