#include "PlayerLockOnUpdater.h"
PlayerLockOnUpdater::PlayerLockOnUpdater(std::function<void(float)> updateFn)
	: update_(std::move(updateFn)) {}

void PlayerLockOnUpdater::Update(float dt) {
	update_(dt);
}