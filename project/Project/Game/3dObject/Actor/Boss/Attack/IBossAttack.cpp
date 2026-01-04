#include "IBossAttack.h"

IBossAttack::IBossAttack()  = default;
IBossAttack::~IBossAttack() = default;

float IBossAttack::GetCooldown() const {return coolDown_;}

bool IBossAttack::Execute(class Boss& boss,class BossShootingController& shootingController) const {
	(void)boss;
	(void)shootingController;
	return true;
}