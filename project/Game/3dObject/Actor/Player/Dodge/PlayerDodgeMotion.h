#pragma once
#include <memory>
#include <cmath>
#include <numbers>
#include <algorithm>

/* engine */
#include "Game/3dObject/Actor/Player/PlayerContext.h"

#include <Engine/Foundation/Math/Quaternion.h>
#include <Engine/Foundation/Math/Vector3.h>

class Player;
class PlayerDodgeSystem;
enum class DodgeState;

class PlayerDodgeMotion {
public:
	PlayerDodgeMotion();
	~PlayerDodgeMotion();

	/**
	 * \brief 初期化
	 * \param ctx		コンテキスト
	 * \param dodge		危機システム
	 */
	void Initialize(const PlayerDodgeContext& ctx,PlayerDodgeSystem* dodge);
	/**
	 * \brief 更新
	 * \param dt	デルタタイム
	 */
	void Update(float dt);
	/**
	 * \brief 回避中か
	 * \param s	回避状態
	 * \return	回避中ならtrue
	 */
	bool IsDodging(DodgeState s);

private:
	/**
	 * \brief 回避開始
	 */
	void OnDodgeStart();
	/**
	 * \brief 回避終了
	 */
	void OnDodgeEnd();
	/**
	 * \brief ジャスト回避成功時
	 */
	void OnPerfect();
	/**
	 * \brief 回転とカーブを適用
	 * \param dt
	 */
	void ApplySpinAndCurve(float dt);
	/**
	 * \brief 手続き的姿勢適用
	 * \param dt
	 */
	void ApplyProceduralPose(float dt);

private:
	PlayerDodgeContext ctx_;
	PlayerDodgeSystem* dodge_ = nullptr;

	// ==== 姿勢系 ====
	Quaternion baseRot_{ 0,0,0,1 }; // 回避開始時の基準姿勢
	Quaternion spinQ_{ 0,0,0,1 };   // そのフレームの絶対スピン回転

	float additiveRoll_  = 0.0f; // Z傾き
	float additivePitch_ = 0.0f; // X傾き
	float leanLerp_      = 0.0f; // 傾きのブレンド係数

	// ==== 位置サーボ ====
	Vector3 appliedOffset_{ 0,0,0 };

	// ==== 沈み（Y） ====
	float sinkCurrent_ = 0.0f;
};