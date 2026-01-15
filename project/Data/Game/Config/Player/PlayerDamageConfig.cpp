#include "PlayerDamageConfig.h"

PlayerDamageConfig::PlayerDamageConfig() {
	AddField("kHitIFrameSec", kHitIFrameSec).Category("damage").Tooltip("被弾無敵時間").Range(0.5f,3.0f);
	AddField("kBlinkHz", kBlinkHz).Category("damage").Tooltip("無敵点滅周波数").Range(1.0f,30.0f);
	AddField("kBlinkInterval", kBlinkInterval).Category("damage").Tooltip("無敵点滅間隔").ReadOnly();
}

CalyxEngine::ParamPath PlayerDamageConfig::GetParamPath() const { return {CalyxEngine::ParamDomain::Game,"PlayerDamage","Player"}; }