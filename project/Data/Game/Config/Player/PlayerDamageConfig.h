#pragma once

// engine
#include <Engine/Foundation/Serialization/SerializableObject.h>

/*-----------------------------------------------------------------------------------------
 * PlayerDamageConfig
 * - プレイヤーダメージ設定構造体
 * - 被弾時の無敵時間・点滅間隔などのパラメータを管理
 *---------------------------------------------------------------------------------------*/
struct PlayerDamageConfig :
	public CalyxEngine::SerializableObject {
public:
	//=================================================================*/
	//  function
	//=================================================================*/
	PlayerDamageConfig();
	CalyxEngine::ParamPath GetParamPath() const override;

	//=================================================================*/
	//  variable
	//=================================================================*/
	// --- 定数 ---
	float kHitIFrameSec;
	float kBlinkHz;
	float kBlinkInterval = 1.0f / kBlinkHz;
};