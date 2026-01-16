#pragma once
#include "Data/Game/Config/Player/PlayerDamageConfig.h"
#include "Game/3dObject/Actor/Player/Context/PlayerContext.h"

#include <memory>

// forward
class Player;
class Collider;

/*-----------------------------------------------------------------------------------------
 * PlayerDamageHandler class
 * - プレイヤーの被弾/無敵/点滅演出を管理するクラス
 * - 被弾時の演出や無敵時間の制御を担当する
 *---------------------------------------------------------------------------------------*/
class PlayerDamageHandler {
public:
	PlayerDamageHandler();
	~PlayerDamageHandler();

	/**
	 * \brief 初期化
	 * \param context コンテキスト
	 */
	void Initialize(const PlayerStateContext& context);
	/**
	 * \brief 更新
	 * \param dt
	 */
	void Update(float dt);
	/**
	 * \brief 衝突時処理
	 * \param other
	 */
	void OnHit(Collider* other);
	/**
	 * \brief 無敵時間付与
	 * \param seconds
	 */
	void SetInvincibleFor(float seconds);
	/**
	 * \brief 無敵中であるかを返す
	 * \return 無敵中であればtrue
	 */
	bool IsInvincible() const;

	/**
	 * \brief
	 * \param seconds
	 */
	void RequestInvincible(float seconds);
	/**
	 * \brief GUI 表示
	 */
	void ShowGUi();
	/**
	 * \brief パラメータ保存
	 */
	void SaveParam();
	/**
	 * \brief パラメータ読み込み
	 */
	void LoadParam();

private:
	/**
	 * \brief 無敵時間更新
	 * \param dt
	 */
	void UpdateInvincibility(float dt);

private:
	PlayerStateContext ctx_;

	PlayerDamageConfig config_;

	// --- 無敵 ---
	float invincibleTimer_       = 0.0f;
	float invincibleBlinkAccum_ = 0.0f;
	bool  invincibleBlinkState_ = true;

};
