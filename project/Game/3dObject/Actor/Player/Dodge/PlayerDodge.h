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
	int   dodgeKey = DIK_LSHIFT;	//< 受付キー
	float distance = 6.0f;			//< 1回の回避距離（移動量）
	float duration = 0.18f;			//< 実移動+i-frame 主区間 (IFrame)
	float startup = 0.06f;			//< 予備動作
	float recovery = 0.14f;			//< 後隙
	float invuln = 0.20f;			//< IFrameの無敵時間（通常は duration と同等でOK）
	float cooldown = 0.35f;			//< 連打防止

	// ジャスト回避判定窓（入力時刻0を中心）
	float perfectWindowBefore = 0.04f;		//< 入力直前の救済
	float perfectWindowAfter = 0.12f;		//< 入力あとの先読み猶予

	bool useCameraForward = true;			//< 方向決定

	// ジャスト成立時、Startup をスキップして即 I-Frame に入る（早送りする）
	bool fastForwardToIFrameOnPerfect = true;
};

enum class DodgeState {
	Idle,
	Startup,
	IFrame,
	Recovery
};

class PlayerDodge {
public:
	PlayerDodge();
	~PlayerDodge();

	using Callback = std::function<void()>;

	void Initialize(Player* owner, const PlayerDodgeConfig& cfg = PlayerDodgeConfig());
	void Update(float dt);

	// 外部（PlayerInputHandler 等）から叩ける
	void RequestDodge();

	// true を返したら「このヒットは無効化済み」（ダメージを通さない）
	bool HandlesHitNow();

	bool IsDodging()  const { return state_ != DodgeState::Idle; }
	bool IsInIFrame() const { return state_ == DodgeState::IFrame; }

	// コールバック（演出フック）
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
	float timer_ = 0.0f;				//< 現在ステートの経過時間
	float cooldown_ = 0.0f;				//< 残りクールダウン

	// 時刻管理（ジャスト判定用）
	float timeAccum_ = 0.0f;			//< 生存累積時刻
	float lastInputTime_ = -9999.0f;	//< 最後に入力が通った時刻

	// 方向
	Vector3 dodgeDir_{ 0,0,1 };

	// 演出フック
	Callback onDodgeStart_{};
	Callback onDodgeEnd_{};
	Callback onPerfectDodge_{};
};
