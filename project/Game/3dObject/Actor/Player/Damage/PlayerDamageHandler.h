#pragma once
#include "Game/3dObject/Actor/Player/PlayerContext.h"

#include <memory>

// forward
class Player;
class Collider;

/**
 * \brief プレイヤーの被弾・無敵・点滅・カメラ演出を管理する
 */
class PlayerDamageHandler {
public:
	PlayerDamageHandler();
	~PlayerDamageHandler();

	/**
	 * \brief 初期化
	 * \param context コンテキスト
	 */
	void Initialize(const PlayerDamageContext& context);
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

private:
	/**
	 * \brief 無敵時間更新
	 * \param dt
	 */
	void UpdateInvincibility(float dt);

private:
	PlayerDamageContext ctx_;

	// --- 無敵 ---
	float invincibleTimer_       = 0.0f;
	float invincibleBlinkAccum_ = 0.0f;
	bool  invincibleBlinkState_ = true;

	// --- 定数 ---
	static constexpr float kHitIFrameSec  = 1.5f;
	static constexpr float kBlinkHz       = 12.0f;
	static constexpr float kBlinkInterval = 1.0f / kBlinkHz;
};