#include "PlayerDamageUpdater.h"

PlayerDamageUpdater::PlayerDamageUpdater(const PlayerDamageContext& ctx)
		: ctx_(std::move(ctx)) {}
PlayerDamageUpdater::~PlayerDamageUpdater() = default;


void PlayerDamageUpdater::ApplyDamage(int value)const {
	int life = ctx_.getLife();
	life -= value;
	ctx_.setLife(life);
}