#include "PlayerDodgeUpdater.h"
#include "../Input/PlayerCommand.h"

PlayerDodgeUpdater::PlayerDodgeUpdater(const PlayerCombatContext& ctx)
	: ctx_(std::move(ctx)) {}

void PlayerDodgeUpdater::Update(const PlayerCommand& cmd, float ) {
	if(cmd.type == PlayerCommandType::Dodge) {
		ctx_.dodge();
	}
}