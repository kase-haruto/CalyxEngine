#pragma once
#include "../BaseBullet.h"
#include <Engine/Application/Effects/FxObject.h>
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

	void Initialize()override;
	void OnShot()override;
	//--------- accessor ---------------------------------------------------
	
private:
	std::weak_ptr<CalyxEffect::FxObject> shootFx_;
};