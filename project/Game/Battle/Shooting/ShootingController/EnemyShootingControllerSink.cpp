#include "EnemyShootingControllerSink.h"

#include "EnemyShootingController.h"

EnemyShootingControllerSink::EnemyShootingControllerSink(EnemyShootingController* ctrl):
	ctrl_(ctrl){}

void EnemyShootingControllerSink::Submit(const ShootBatch& batch){
	if (!ctrl_)return;

	for (auto& s : batch.shots){ ctrl_->RequestShoot(s.origin,s.dirN); }
}