#include "ClearHudConfig.h"


ClearLogoHudConfig::ClearLogoHudConfig()  {}

CalyxEngine::ParamPath ClearLogoHudConfig::GetParamPath() const  {
	return {CalyxEngine::ParamDomain::Game, "ClearLogoHud"};
}