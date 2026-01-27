#pragma once

#include "Data/Game/Config/Player/PlayerDodgeConfig.h"
#include "Game/3dObject/Actor/Player/Context/PlayerContext.h"

#include <Engine/Foundation/Input/Input.h>
#include <Engine/Foundation/Math/Vector3.h>
#include <functional>
enum class DodgeState {
	Idle,
	Startup,
	IFrame,
	Recovery
};

class Player; // 前方宣言

/*-----------------------------------------------------------------------------------------
 * PlayerDodgeSystem class
 * - プレイヤーの回避状態を管理するクラス
 * - 回避の入力受付や無敵付与などの挙動を制御する
 *---------------------------------------------------------------------------------------*/
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
	void Initialize();
	/**
	 * \brief 更新
	 * \param dt デルタタイム
	 */
	void Update(float dt);
	/**
	 * \brief 外部からの回避Requestを受ける
	 * \param dir 回避方向
	 */
	void RequestDodge(const CalyxMath::Vector3& dir);

	/**
	 * \brief
	 * \return
	 */
	CalyxMath::Vector3 GetDodgeVelocity() const;

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

	void ShowGui();
	void SaveConfig();
	void LoadConfig();

	// accessor -------------------------------------------------------//
	bool					  IsDodging() const;
	bool					  IsInIFrame() const;
	DodgeState				  GetState() const;
	float					  GetStateTime() const;
	const CalyxMath::Vector3& GetDodgeDir() const;
	const PlayerDodgeConfig&  GetConfig() const;

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
	PlayerDodgeConfig  cfg_{};					  //< 設定
	DodgeState		   state_ = DodgeState::Idle; //< 現在の状態
	CalyxMath::Vector3 dodgeDir_{0, 0, 1};		  //< 回避方向

	bool isJustDodge_ = false; //< ジャスト回避中か

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
