#include "PlayerMoveUpdater.h"


PlayerMoveUpdater::PlayerMoveUpdater(const PlayerMoveContext& ctx) 
		: ctx_(ctx) {}

PlayerMoveUpdater::~PlayerMoveUpdater() = default;

/////////////////////////////////////////////////////////////////////////////////////
//			更新
/////////////////////////////////////////////////////////////////////////////////////
void PlayerMoveUpdater::Update(const PlayerCommand& cmd, float dt) {
	if(cmd.type != PlayerCommandType::Move) return;

	if(auto* m = std::get_if<CmdMove>(&cmd.value)) {
		const float speed = ctx_.getMoveSpeed();
		// 移動と傾き処理
		ctx_.addMove(m->delta * speed * dt);
		ctx_.updateTilt(m->delta);
	}
}