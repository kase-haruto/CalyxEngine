#include "IBossAttack.h"

IBossAttack::IBossAttack()  = default;
IBossAttack::~IBossAttack() = default;

bool IBossAttack::Execute(class Boss& boss,class BossShootingController& shootingController) const {
	(void)boss;
	(void)shootingController;
	return true;
}

/////////////////////////////////////////////////////////////////
// パラメータセットアップ
/////////////////////////////////////////////////////////////////
IBossAttackParam::IBossAttackParam() {
	AddField("cooldown",cooldown).Category("BossAttack");
}

CalyxEngine::ParamPath IBossAttackParam::GetParamPath() const {
	return {CalyxEngine::ParamDomain::Game, attackName , serializeSubRootPath_};
}