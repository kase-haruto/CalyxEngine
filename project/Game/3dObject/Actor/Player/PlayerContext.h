#pragma once
#include "Engine/Objects/Transform/Transform.h"

#include <Engine/Foundation/Math/Vector3.h>
#include <functional>

struct PlayerMoveContext {
	std::function<void(const Vector3&)> addMove;
	std::function<void(const Vector3&)> updateTilt;
	std::function<float()>				getMoveSpeed;
};

struct PlayerCombatContext {
	std::function<void()> shoot;
	std::function<void()> dodge;
	std::function<void()> lockOn;
	std::function<void()> clearLockOn;
};

struct PlayerDodgeContext {
	std::function<WorldTransform()> getWorldTransform;
	std::function<void(const Vector3&)> addMoveRequest;
};

struct PlayerDamageContext {
	std::function<int()>	  getLife;
	std::function<void(int)>  setLife;
	std::function<void(bool)> setVisible;
};

struct PlayerReticleContext {
	std::function<void(const Vector3&)> moveReticle;
};

struct PlayerLockOnContext {
	std::function<Vector3()> getReticleWorldPos;
};

struct DangerSenseContext {
	std::function<Vector3()> getPlayerCenter;
	std::function<float()>   getPlayerRadius;
	std::function<void(bool)> setPerfectDodgeHint;
};