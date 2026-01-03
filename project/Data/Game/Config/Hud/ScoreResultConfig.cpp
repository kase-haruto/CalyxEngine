#include "ScoreResultConfig.h"

ScoreResultConfig::ScoreResultConfig() {
	AddField("delaySec",     delaySec);
	AddField("countUpSec",   countUpSec);
	AddField("countEaseInt", countEaseInt);
}

CalyxEngine::ParamPath ScoreResultConfig::GetParamPath() const { return {CalyxEngine::ParamDomain::Game,"ScoreResultHud"}; }