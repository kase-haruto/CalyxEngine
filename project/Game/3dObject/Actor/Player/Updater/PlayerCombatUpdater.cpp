#include "PlayerCombatUpdater.h"
#include "Game/3dObject/Actor/Player/Input/PlayerCommand.h"

PlayerCombatUpdater::PlayerCombatUpdater(const PlayerCombatContext& ctx) 
		: ctx_(std::move(ctx)) {}

void PlayerCombatUpdater::Update(const PlayerCommand& cmd, float ) {
	switch(cmd.type) {

		///// 発射 /////
	case PlayerCommandType::Shoot:
		ctx_.shoot();
		break;

		///// ロックオン /////
	case PlayerCommandType::LockOn:
		ctx_.lockOn();
		break;
		
		///// ロックオン解除 /////
	case PlayerCommandType::ClearLockOn:
		ctx_.clearLockOn();
		break;

	default:
		break;
	}
}