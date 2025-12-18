#pragma once

#include "Game/3dObject/Actor/Player/Context/PlayerContext.h"

#include <Engine/Application/Input/Input.h>
#include <Engine/Foundation/Math/Vector3.h>
#include <functional>

struct PlayerDodgeConfig {
	int	  dodgeKey = DIK_LSHIFT;
	float distance = 10.0f;
	float duration = 0.18f;
	float startup  = 0.06f;
	float recovery = 0.14f;
	float invuln   = 0.20f;
	float cooldown = 0.35f;

	float perfectWindowBefore = 0.04f;
	float perfectWindowAfter  = 0.08f;

	bool useCameraForward = true;

	bool  useCustomCurve	 = true; // IFrame直進を止め、モーション側に任せる
	float spinTurns			 = 1.0f; // Y軸回転回数（1.0=一回転）
	float lateralScale		 = 0.0f; // 横移動
	float backwardScale		 = 2.0f; // 後ろ移動の強さ
	float perfectInvulnBonus = 0.2f; // 回避成功時のボーナス無敵時間
};

enum class DodgeState {
	Idle,
	Startup,
	IFrame,
	Recovery
};

class Player; // 前方宣言

/**
 * \brief playerの回避システム
 */
class PlayerDodgeSystem {
	using Callback = std::function<void()>;

public:
	//=====================================================================*/
	//			public methods
	//=====================================================================*/
	PlayerDodgeSystem();
	~PlayerDodgeSystem();

	/**
	 * \brief 初期化
	 * \param owner プレイヤー
	 * \param cfg 回避設定
	 */
	void Initialize(const PlayerDodgeConfig& cfg);
	/**
	 * \brief 更新
	 * \param dt デルタタイム
	 */
	void Update(float dt);
	/**
	 * \brief 外部からの回避Requestを受ける
	 */
	void RequestDodge();

	/**
	 * \brief ヒントuiのアクティブ設定
	 * \param v
	 */
	void SetPerfectHintActive(bool v);
	/**
	 * \brief 今回の回避がパーフェクト回避になるか
	 * \return
	 */
	bool WouldBePerfectIfDodgedNow() const;

	// コールバック
	void SetOnDodgeStart(Callback cb);
	void SetOnDodgeEnd(Callback cb);
	void SetOnPerfectDodge(Callback cb);
	void SetOnRequestInvincible(std::function<void(float)> fn);

	// accessor -------------------------------------------------------//
	bool					 IsDodging() const;
	bool					 IsInIFrame() const;
	DodgeState				 GetState() const;
	float					 GetStateTime() const;
	const Vector3&			 GetDodgeDir() const;
	const PlayerDodgeConfig& GetConfig() const;

private:
	//=====================================================================*/
	//			private methods
	//=====================================================================*/
	/**
	 * \brief 状態を変更
	 * \param next
	 */
	void ChangeState(DodgeState next);

private:
	//=====================================================================*/
	//			private variables
	//=====================================================================*/
	PlayerDodgeConfig  cfg_{}; //< 設定
	DodgeState		   state_ = DodgeState::Idle; //< 現在の状態
	Vector3			   dodgeDir_{0, 0, 1};		  //< 回避方向

	float timer_		 = 0.0f;	 //< 状態経過時間
	float cooldown_		 = 0.0f;	 //< 回避クールダウン
	float timeAccum_	 = 0.0f;	 //< 入力受付時間累積
	float lastInputTime_ = -9999.0f; //< 最後に入力を受け付けた時間

	bool perfectHintActive_ = false; //< パーフェクト回避ヒントがアクティブか

	Callback onDodgeStart_{};	//< 回避開始コールバック
	Callback onDodgeEnd_{};		//< 回避終了コールバック
	Callback onPerfectDodge_{}; //< パーフェクト回避コールバック

	std::function<void(float)> onRequestInvincible_; //< 無敵リクエストコールバック
};