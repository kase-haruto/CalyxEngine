#pragma once
#include <Engine/Foundation/Serialization/SerializableObject.h>

/*-----------------------------------------------------------------------------------------
 * PlayerDodgeConfig
 * - プレイヤー回避設定構造体
 * - 回避の距離・持続時間・無敵時間などのパラメータを管理
 *---------------------------------------------------------------------------------------*/
struct PlayerDodgeConfig : public CalyxEngine::SerializableObject {

	PlayerDodgeConfig();

	CalyxEngine::ParamPath GetParamPath() const override;

	size_t dodgeKey;
	float  distance;
	float  duration;
	float  startup;
	float  recovery;
	float  invuln;
	float  cooldown;

	float perfectWindowBefore;
	float perfectWindowAfter;

	bool useCameraForward = true;

	bool  useCustomCurve;	  // IFrame直進を止め、モーション側に任せる
	float spinTurns;		  // Y軸回転回数（1.0=一回転）
	float lateralScale;		  // 横移動
	float backwardScale;	  // 後ろ移動の強さ
	float perfectInvulnBonus; // 回避成功時のボーナス無敵時間

	float perfectSlowTimeScale; // 回避成功時のスロー倍率
	float perfectSlowDuration;	// 回避成功時のスロー持続時間
};