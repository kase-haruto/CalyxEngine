#include "PlayerReticleUpdater.h"

PlayerReticleUpdater::PlayerReticleUpdater(const PlayerReticleContext& ctx) : ctx_(ctx) {}

//////////////////////////////////////////////////////////////////////
//  更新
//////////////////////////////////////////////////////////////////////
void PlayerReticleUpdater::Update(const PlayerCommand& cmd, float /*dt*/) {
	if(cmd.type != PlayerCommandType::MoveReticle) {
		return;
	}

	if(auto* m = std::get_if<CmdMove>(&cmd.value)) {
		ctx_.moveReticle(m->delta);
	}
}
