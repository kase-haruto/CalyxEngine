#pragma once
#include "../BaseBullet.h"
#include <Engine/Application/Effects/Particle/Object/ParticleSystemObject.h>

class PlayerBullet :
    public BaseBullet{
public:
	//===================================================================*/
	//			public function
	//===================================================================*/
	PlayerBullet() = default;
	PlayerBullet(const std::string& modelName,const std::string& name);
	~PlayerBullet()override;

	void Update() override;
	//--------- accessor ---------------------------------------------------
	
private:
	std::unique_ptr<ParticleSystemObject> trailFx_;
	std::unique_ptr<ParticleSystemObject> shootFx_;
};

