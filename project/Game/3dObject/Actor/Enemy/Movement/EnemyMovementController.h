#pragma once
#include "Game/Battle/Movement/Formation/EnemyFormationController.h"

#include <Engine/Foundation/Math/Vector3.h>
#include <Game/Battle/Movement/FollowSpline/SplineFollower.h>

class Enemy;
class WorldTransform;
class EnemyFormationController;

/*-----------------------------------------------------------------------------------------
 * EnemyMovementController class
 * - 敵の移動モードを管理するクラス
 * - 進入/滞在/退場/編隊移動などの挙動を制御する
 *---------------------------------------------------------------------------------------*/
class EnemyMovementController {
public:
	enum class Mode {
		Active,
		StayInView,
		ExitFromView,
		Formation,	//< 隊列モード
		Dissolving, //< 退場エフェクト中
		Entrance,	//< 進入エフェクト中
	};

public:
	EnemyMovementController();
	~EnemyMovementController() = default;

	/**
	 * \brief 初期化
	 * \param owner 所有者 Enemy
	 */
	void Initialize(Enemy* owner);
	/**
	 * \brief 更新
	 * \param dt デルタタイム
	 */
	void Update(float dt);
	void UpdateDissolve(float dt);
	/**
	 * \brief カメラ内ステイ開始
	 * \param duration ステイ時間
	 */
	void StartStay(float duration);
	/**
	 * \brief アクティブモード開始
	 */
	void StartActive();
	/**
	 * \brief カメラ外退場開始
	 */
	void BeginExit();
	/**
	 * \brief 退場エフェクト開始
	 * \param formationIndex 編隊内インデックス（0～N）
	 */
	void StartDissolve(int index, DissolvePattern pattern);

	void StartEntranceToFormation(
		EnemyFormationController* formation,
		const CalyxMath::Vector3&			  formationOffset,
		const CalyxMath::Vector3&			  entranceStartWorld);

	//  accessor -------------------------------------------------------------//
	void SetRoute(const SplineData& route, const WorldTransform* playerTf);
	void SetPlayerTransform(const WorldTransform* playerTf);
	Mode GetMode() const { return mode_; }

private:
	/**
	 * \brief ステイ中更新
	 * \param dt デルタタイム
	 */
	void UpdateStay(float dt);
	/**
	 * \brief 退場中更新
	 * \param dt デルタタイム
	 */
	void UpdateExit(float dt);
	/**
	 * \brief アクティブ中更新
	 * \param dt デルタタイム
	 */
	void UpdateActive(float dt);
	/**
	 * \brief カメラドリフト更新
	 * \param dt デルタタイム
	 */
	void UpdateCameraDrift(float dt);
	void UpdateFormation(float);
	/**
	 * \brief プレイヤーを注視
	 */
	void LookAtPlayer();
	/**
	 * \brief 退場完了チェック
	 * \return 退場完了したら true
	 */
	bool CheckExitFinished() const;

	void UpdateEntrance(float dt);

private:
	Enemy* owner_ = nullptr;

	Mode mode_ = Mode::Active;

	float stayTimer_   = 0.0f;
	float maxStayTime_ = 3.0f;

	// ドリフト
	float camPhaseX_ = 0, camPhaseY_ = 0, camPhaseZ_ = 0;
	float driftAmpX_ = 15.0f, driftAmpY_ = 15.5f, driftAmpZ_ = 15.0f;
	float driftFreqX_ = 0.7f, driftFreqY_ = 0.85f, driftFreqZ_ = 0.35f;
	float driftMargin_ = 0.4f;

	CalyxMath::Vector3 camAnchor_ = {0, 0, 40};

	// Exit
	bool	exitPrepared_  = false;
	CalyxMath::Vector3 exitDirLocal_  = {0, 0, 0};
	float	exitSpeed_	   = 35.0f;
	float	exitOvershoot_ = 1.05f;

	// Spline
	SplineFollower mover_;
	SplineData	   moveRoute_;
	bool		   hasRoute_ = false;

	const WorldTransform* playerTf_ = nullptr;

	// Formation
	EnemyFormationController* formation_	   = nullptr; // 非所有
	CalyxMath::Vector3					  formationOffset_ = {0, 0, 0};
	float					  formationPhase_  = 0.0f; // 個体差の揺れ用

	// dissolve 用の速度ベクトル
	CalyxMath::Vector3 scatterVelocity_ = {0, 0, 0};
	int		formationSize_	 = 1;

	int formationIndex_ = 0;

	// 侵入用
	CalyxMath::Vector3 entranceStart_	= {0, 0, 0}; // 開始位置（画面外）
	CalyxMath::Vector3 entranceTarget_ = {0, 0, 0}; // 合流目標（編隊オフセット位置）
	float	entranceTime_	= 0.0f;
	float	entranceLength_ = 2.0f; // 侵入にかける時間（秒）
};
