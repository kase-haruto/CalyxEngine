#include "ClearHudConfig.h"


ClearLogoHudConfig::ClearLogoHudConfig()  {
	easeTypeInt = static_cast<int32_t>(easeType);
	AddField("startPosition", startPosition);
	AddField("stayPosition", stayPosition);
	AddField("scale", scale);
	AddField("duration", duration);
	AddField("easeType", easeTypeInt);

	// AddField 後に設定が上書きされている可能性があるので
	// int 側の値を enum に戻して同期する
	easeType = static_cast<CalyxEase::EaseType>(easeTypeInt);
}

CalyxEngine::ParamPath ClearLogoHudConfig::GetParamPath() const  {
	return {CalyxEngine::ParamDomain::Game, "ClearLogoHud"};
}