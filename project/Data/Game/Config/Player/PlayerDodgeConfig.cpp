#include "PlayerDodgeConfig.h"

PlayerDodgeConfig::PlayerDodgeConfig() {
	AddField("dodgeKey", dodgeKey).Category("dodge").ReadOnly();
	AddField("duration", duration).Category("dodge").Range(0.0f, 0.5f);
	AddField("startup", startup).Category("dodge").Range(0.0f, 0.5f);
	AddField("recovery", recovery).Category("dodge").Range(0.0f, 0.5f);
	AddField("invuln", invuln).Category("dodge").Range(0.0f, 1.0f);
	AddField("cooldown", cooldown).Category("dodge").Range(0.0f, 1.0f);
	AddField("distance", distance).Category("dodge").Range(1.0f, 50.0f);
	AddField("perfectWindowBefore", perfectWindowBefore).Category("dodge").Range(0.0f, 0.5f);
	AddField("perfectWindowAfter", perfectWindowAfter).Category("dodge").Range(0.0f, 0.5f);
	AddField("useCameraForward", useCameraForward).Category("dodge");
	AddField("useCustomCurve", useCustomCurve).Category("dodge");
	AddField("spinTurns", spinTurns).Category("dodge").Range(0.0f, 5.0f);
	AddField("lateralScale", lateralScale).Category("dodge").Range(0.0f, 2.0f);
	AddField("backwardScale", backwardScale).Category("dodge").Range(0.0f, 5.0f);
	AddField("perfectInvulnBonus", perfectInvulnBonus).Category("dodge").Range(0.0f, 1.0f);

	perfectSlowTimeScale = 0.2f;
	perfectSlowDuration	 = 0.8f;
	AddField("perfectSlowTimeScale", perfectSlowTimeScale).Category("dodge").Range(0.0f, 1.0f);
	AddField("perfectSlowDuration", perfectSlowDuration).Category("dodge").Range(0.0f, 3.0f);
}

CalyxEngine::ParamPath PlayerDodgeConfig::GetParamPath() const { return {CalyxEngine::ParamDomain::Game, "PlayerDodge", "Player"}; }