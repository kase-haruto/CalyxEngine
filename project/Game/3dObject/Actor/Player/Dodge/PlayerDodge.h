#pragma once
/* engine */
#include <Engine/Foundation/Math/Vector3.h>
#include <Engine/Application/Input/Input.h>
#include <Engine/Graphics/Camera/Manager/CameraManager.h>
#include <Engine/Foundation/Clock/ClockManager.h>

/* c++ */
#include <functional>

class Player; // 前方宣言

struct PlayerDodgeConfig {
	int dodgeKey = DIK_LSHIFT;
	float distance = 10.0f;
	float duration = 0.18f;
	float startup = 0.06f;
	float recovery = 0.14f;
	float invuln = 0.20f;
	float cooldown = 0.35f;

	float perfectWindowBefore = 0.04f;
	float perfectWindowAfter = 0.08f;

	bool  useCameraForward = true;

	bool  useCustomCurve = true;   // IFrame直進を止め、モーション側に任せる
	float spinTurns = 1.0f;   // Y軸回転回数（1.0=一回転）
	float lateralScale = 0.0f;   // 横移動
	float backwardScale = 2.0f;  // 後ろ移動の強さ
	float perfectInvulnBonus = 0.2f;	//回避成功時のボーナス無敵時間
};


enum class DodgeState {
	Idle,
	Startup,
	IFrame,
	Recovery
};

class PlayerDodge {
public:
	using Callback = std::function<void()>;

	void Initialize(Player* owner, const PlayerDodgeConfig& cfg = PlayerDodgeConfig());
	void Update(float dt);

	void RequestDodge();

	void SetPerfectHintActive(bool v) { perfectHintActive_ = v; }

	bool WouldBePerfectIfDodgedNow() const { return perfectHintActive_; }

	bool HandlesHitNow();

	bool IsDodging()  const { return state_ != DodgeState::Idle; }
	bool IsInIFrame() const { return state_ == DodgeState::IFrame; }

		// モーション側が参照する情報
	DodgeState GetState() const			{ return state_; }
	float GetStateTime() const			{ return timer_; }
	const Vector3& GetDodgeDir()  const	{ return dodgeDir_; }
	const PlayerDodgeConfig& Cfg() const{ return cfg_; }

	void SetOnDodgeStart(Callback cb) { onDodgeStart_ = std::move(cb); }
	void SetOnDodgeEnd(Callback cb) { onDodgeEnd_ = std::move(cb); }
	void SetOnPerfectDodge(Callback cb) { onPerfectDodge_ = std::move(cb); }

	const PlayerDodgeConfig& GetConfig() const { return cfg_; }
	void SetConfig(const PlayerDodgeConfig& c) { cfg_ = c; }

private:
	void ChangeState(DodgeState next);
	void MoveOwnerBy(const Vector3& velocity);

private:
	Player* owner_ = nullptr;
	PlayerDodgeConfig cfg_{};

	DodgeState state_{ DodgeState::Idle };
	float timer_ = 0.0f;
	float cooldown_ = 0.0f;

	float timeAccum_ = 0.0f;
	float lastInputTime_ = -9999.0f;

	Vector3 dodgeDir_{ 0,0,1 };

	bool perfectHintActive_ = false;

	Callback onDodgeStart_{};
	Callback onDodgeEnd_{};
	Callback onPerfectDodge_{};
};
