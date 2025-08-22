#pragma once

// engine
#include <Engine/Application/Effects/Particle/Object/ParticleSystemObject.h>

// game
#include <Game/3dObject/Actor/Bullet/BaseBullet.h>

class EnemyBullet
	: public BaseBullet{
public:
	//===================================================================*/
	//		public methods
	//===================================================================*/
	EnemyBullet() = default;
	EnemyBullet(const std::string& modelName, const std::string& name);
	~EnemyBullet()override;

	void Initialize()override;
	void OnShot()override;
	//--------- accessor ---------------------------------------------------
	
private:
	std::shared_ptr<ParticleSystemObject> trailFx_;
	std::shared_ptr<ParticleSystemObject> shootFx_;
};
