#pragma once
/* ========================================================================
/*   include space
/* ===================================================================== */
// engine
#include <Engine/Application/Effects/Particle/Object/ParticleSystemObject.h>

// game
#include "Engine/Application/Effects/FxObject.h"

#include <Game/3dObject/Actor/Bullet/BaseBullet.h>

class BossBullet
	: public BaseBullet {
public:
	//===================================================================*/
	//		public methods
	//===================================================================*/
	BossBullet() = default;
	BossBullet(const std::string& modelName, const std::string& name);
	~BossBullet()override;

	void Initialize()override;
	void OnShot()override;
	//--------- accessor ---------------------------------------------------

private:
	std::weak_ptr<FxObject> shootFx_;

};