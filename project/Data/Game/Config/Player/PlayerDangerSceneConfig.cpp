#include "PlayerDangerSceneConfig.h"

DangerSenseConfig::DangerSenseConfig() {
	AddField("playerInflate", playerInflate).Category("dangerSense").Tooltip("プレイヤー当たり判定拡大率").Range(0.0f, 3.0f);
	AddField("margin", margin).Category("dangerSense").Tooltip("障害物当たり判定マージン").Range(0.0f, 10.0f);
	AddField("maxCheckDistance", maxCheckDistance).Category("dangerSense").Tooltip("最大検出距離").Range(10.0f, 300.0f);
	AddField("throttleFrames", throttleFrames).Category("dangerSense").Tooltip("検出間隔フレーム数").Range(1, 60);
	AddField("graceTime", graceTime).Category("dangerSense").Tooltip("回避猶予時間").Range(0.0f, 2.0f);
}


CalyxEngine::ParamPath DangerSenseConfig::GetParamPath() const { return {CalyxEngine::ParamDomain::Game,"PlayerDangerSense","Player"}; }