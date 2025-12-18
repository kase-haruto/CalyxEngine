#include "PlayerContextBuilder.h"
#include "Game/3dObject/Actor/Player/Dodge/PlayerDodgeSystem.h"
#include "Game/3dObject/Actor/Player/Player.h"

PlayerContextBuilder::PlayerContextBuilder(Player& player)
	: player_(player) {}

PlayerActionContext PlayerContextBuilder::BuildAction() {
	Player& p = player_;

	return PlayerActionContext{
		.addMove	  = [&p](const Vector3& v) { p.AddMoveRequest(v); },
		.updateTilt	  = [&p](const Vector3& v) { p.UpdateTilt(v); },
		.getMoveSpeed = [&p]() -> float {
			return p.GetMoveSpeed();
		},
		.moveReticle   = [&p](const Vector3& v) { p.MoveReticle(v); },
		.getReticlePos = [&p]() -> Vector3 {
			return p.GetReticleWorldPos();
		},
		.shoot		 = [&p]() { p.RequestShoot(); },
		.dodge		 = [&p]() { p.RequestDodge(); },
		.lockOn		 = [&p]() { p.RequestLockOn(); },
		.clearLockOn = [&p]() { p.RequestLockOnTargetClear(); }};
}

PlayerStateContext PlayerContextBuilder::BuildState() {
	Player& p = player_;

	return PlayerStateContext{
		.getCenterPos		  = [&p]() { return p.GetCenterPos(); },
		.getCollisionRadius	  = [&p]() { return p.GetCollisionRadius(); },
		.getLife			  = [&p]() { return p.GetLife(); },
		.setLife			  = [&p](int v) { p.SetLife(v); },
		.setVisible			  = [&p](bool v) { p.SetDrawEnable(v); },
		.setPerfectHintActive = [&p](bool v) {
			if (auto* dodge = p.GetDodgeSystem()) {
				dodge->SetPerfectHintActive(v);
			} },
	};
}
