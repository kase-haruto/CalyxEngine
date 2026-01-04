#include "ClearHudConfig.h"


ClearLogoHudConfig::ClearLogoHudConfig() {
	AddField("logoSize", logoSize);
	AddField("amplitude", amplitude);
	AddField("period", period);
}

CalyxEngine::ParamPath ClearLogoHudConfig::GetParamPath() const  {
	return {CalyxEngine::ParamDomain::Game, "ClearLogoHud"};
}