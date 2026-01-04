#include "DimFilterConfig.h"

DimFilterConfig::DimFilterConfig() {
	AddField("startAlpha", startAlpha);
	AddField("targetAlpha", targetAlpha);
	AddField("duration",  duration);
}

CalyxEngine::ParamPath DimFilterConfig::GetParamPath() const {
	return {CalyxEngine::ParamDomain::Game, name};
}