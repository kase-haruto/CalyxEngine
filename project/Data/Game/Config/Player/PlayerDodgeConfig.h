#pragma once
#include <Engine/Foundation/Input/Input.h>
#include <Engine/Foundation/Serialization/SerializableObject.h>

struct PlayerDodgeConfig :
	public CalyxEngine::SerializableObject {

	PlayerDodgeConfig();

	CalyxEngine::ParamPath GetParamPath() const override;

	size_t   dodgeKey = DIK_LSHIFT;
	float distance = 10.0f;
	float duration = 0.18f;
	float startup  = 0.06f;
	float recovery = 0.14f;
	float invuln   = 0.20f;
	float cooldown = 0.35f;

	float perfectWindowBefore = 0.04f;
	float perfectWindowAfter  = 0.08f;

	bool useCameraForward = true;

	bool  useCustomCurve     = true; // IFrame直進を止め、モーション側に任せる
	float spinTurns          = 1.0f; // Y軸回転回数（1.0=一回転）
	float lateralScale       = 0.0f; // 横移動
	float backwardScale      = 2.0f; // 後ろ移動の強さ
	float perfectInvulnBonus = 0.2f; // 回避成功時のボーナス無敵時間
};