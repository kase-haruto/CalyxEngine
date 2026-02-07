#include "PlayerLockOnConfig.h"

PlayerLockOnConfig::PlayerLockOnConfig() {
	AddField("maxLockOn", maxLockOn_).Category("lockon").Tooltip("MaxLockon").Range(1, 20);
	AddField("lockOnRadiusPx", lockOnRadiusPx_).Category("lockon").Tooltip("ロックオン表示半径(px)").Range(10.0f, 500.0f);
	AddField("lockOnAcquireRadiusPx", lockOnAcquireRadiusPx_).Category("lockon").Tooltip("ロックオン獲得半径(px)").Range(10.0f, 500.0f);
	AddField("lockOnReleaseRadiusPx", lockOnReleaseRadiusPx_).Category("lockon").Tooltip("ロックオン解除半径(px)").Range(10.0f, 1000.0f);
	AddField("lockOnRefreshInterval", lockOnRefreshInterval_).Category("lockon").Tooltip("ロックオン判定間隔（秒）").Range(0.01f, 1.0f);
	AddField("lockOnRefreshTimer", lockOnRefreshTimer_).Category("lockon").Tooltip("ロックオン判定タイマー").ReadOnly();
}

CalyxEngine::ParamPath PlayerLockOnConfig::GetParamPath() const { {return {CalyxEngine::ParamDomain::Game, "PlayerLockOn","Player"}; } }