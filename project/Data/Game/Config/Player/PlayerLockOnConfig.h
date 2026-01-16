#pragma once

#include <Engine/Foundation/Serialization/SerializableObject.h>

/*-----------------------------------------------------------------------------------------
 * PlayerLockOnConfig
 * - プレイヤーロックオン設定構造体
 * - ロックオン判定の半径・間隔・最大数などのパラメータを管理
 *---------------------------------------------------------------------------------------*/
struct PlayerLockOnConfig :
	public CalyxEngine::SerializableObject {
public:
	PlayerLockOnConfig();

	CalyxEngine::ParamPath GetParamPath() const;

	size_t maxLockOn_ = 5;					//< 最大ロックオン数

	float lockOnRadiusPx_        = 60.0f;	//< ロックオン表示半径(px)
	float lockOnAcquireRadiusPx_ = 60.0f;	//< ロックオン獲得半径(px)
	float lockOnReleaseRadiusPx_ = 400.0f;	//< ロックオン解除半径(px)
	float lockOnRefreshInterval_ = 0.15f;	//< ロックオン判定間隔（秒）
	float lockOnRefreshTimer_    = 0.0f;	//< ロックオン判定タイマー
};