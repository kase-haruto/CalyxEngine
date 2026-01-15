#include "TitleMenueConfig.h"

TitleMenueConfig::TitleMenueConfig() {
	AddField("basePos",basePos_);
	AddField("baseSize",baseSize_);
	AddField("space",space_);
	AddField("enlargedScale",enlargedScale_);
	AddField("animSpeed",animSpeed_);
	AddField("useBackEase",useBackEase_);
}

CalyxEngine::ParamPath TitleMenueConfig::GetParamPath() const {return {CalyxEngine::ParamDomain::Game, "TitleMenue"}; }