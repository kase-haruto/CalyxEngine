#pragma once

#include <Game/Battle/Shooting/Details/IShootSink.h>

class EnemyShootingController;

class EnemyShootingControllerSink
		: public IShootSink{
public:
	//===================================================================*/
	//					public methods
	//===================================================================*/
	EnemyShootingControllerSink(EnemyShootingController* ctrl);
	void Submit(const ShootBatch& batch)override;

private:
	//===================================================================*/
	//					private methods
	//===================================================================*/
	EnemyShootingController* ctrl_;
};