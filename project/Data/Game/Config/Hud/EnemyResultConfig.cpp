#include "EnemyResultConfig.h"

EnemyResultConfig::EnemyResultConfig() {
	//====================================================
	// 共通（
	//====================================================
	AddField("delaySec",     delaySec);
	AddField("countUpSec",   countUpSec);
	AddField("countEaseInt", countEaseInt);

	//====================================================
	// EnemyResult 固有
	//====================================================
	AddField("rowSpacing",     rowSpacing);
	AddField("perRowDelaySec", perRowDelaySec);

	AddField("iconOffset",   iconOffset);
	AddField("numberOffset", numberOffset);

	AddField("iconSize",  iconSize);
	AddField("digitSize", digitSize);

}

CalyxEngine::ParamPath EnemyResultConfig::GetParamPath() const {
	return { CalyxEngine::ParamDomain::Game, "EnemyResultHud" };
}