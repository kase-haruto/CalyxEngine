#pragma once
#include "Engine/Objects/Transform/Transform.h"

#include <Engine/Foundation/Math/Vector3.h>
#include <functional>

struct PlayerActionContext {
	// movement
	std::function<void(const CxMath::Vector3&)> addMove;
	std::function<void(const CxMath::Vector3&)> updateTilt;
	std::function<float()>				getMoveSpeed;

	// reticle
	std::function<void(const CxMath::Vector3&)> moveReticle;
	std::function<CxMath::Vector3()>   getReticlePos;

	// combat
	std::function<void()> shoot;
	std::function<void()> dodge;
	std::function<void()> lockOn;
	std::function<void()> clearLockOn;
};

/**
 * \brief プレイヤーの状態参照・操作用コンテキスト
 */
struct PlayerStateContext {
	// spatial state
	std::function<CxMath::Vector3()> getCenterPos;
	std::function<float()>	 getCollisionRadius;

	// life
	std::function<int()>	 getLife;
	std::function<void(int)> setLife;

	// visibility
	std::function<void(bool)> setVisible;
	std::function<void(bool)> setPerfectHintActive;
};
