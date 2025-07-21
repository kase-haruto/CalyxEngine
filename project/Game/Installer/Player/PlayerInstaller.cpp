#include "PlayerInstaller.h"

#include <Game/3dObject/Actor/Bullet/Container/PlayerBulletContainer.h>
#include <Game/Input/PlayerInput/PlayerInputHandler.h>

#include <Engine/Scene/Utility/SceneUtility.h>

std::shared_ptr<Player> PlayerInstaller::InstallPlayer(){
	std::shared_ptr<Player> player;
	player = SceneAPI::Instantiate<Player>("player.gltf", "player");
	
	// shoot
	auto playerBulletContainer_ = SceneAPI::Instantiate<PlayerBulletContainer>("playerBulletContainer");
	auto playerShootingController_ = std::make_unique<PlayerShootingController>(playerBulletContainer_.get());

	// input
	auto playerInput = std::make_unique<PlayerInputHandler>();

	player->SetShootingController(std::move(playerShootingController_));
	player->SetInputHandler(std::move(playerInput));

	return player;
}
