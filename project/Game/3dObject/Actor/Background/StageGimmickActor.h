#pragma once

/// ===================================================================== */
///  include space
/// ===================================================================== */
#include "BackgroundActor.h"

enum class GimmickState {
	Idle = 0, //< 未動作
	Running,  //< 動作中
	Finished  //< 動作終了
};

/*-----------------------------------------------------------------------------------------
 * StageGimmickActor
 * - ステージギミックの基底クラス
 * - ステージギミックの共通インターフェースや基本機能を提供
 * - 具体的なステージギミックはこのクラスを継承して実装する
 *---------------------------------------------------------------------------------------*/
class StageGimmickActor
	: public BackgroundActor {
public:
	//====================================================================*/
	//			public methods
	//====================================================================*/
	StageGimmickActor();
	~StageGimmickActor() override;

	/**
	 * \brief 更新処理
	 * \param dt デルタタイム
	 */
	void Update(float dt) override;

	// accessor ------------------------------
	// getter
	GimmickState GetCurrentState() const { return currentState_; }
	// setter
	void SetCurrentState(GimmickState state) { currentState_ = state; }

protected:
	//====================================================================*/
	//			protected methods
	//====================================================================*/
	// 各状態の処理
	/**
	 * \brief 未動作時の処理
	 * \param dt
	 */
	virtual void IdleUpdate(float dt);
	/**
	 * \brief トリガーされた瞬間の処理
	 * \param dt
	 */
	virtual void OnTriggered();
	/**
	 * \brief 動作中の処理
	 * \param dt
	 */
	virtual void RunningUpdate(float dt);
	/**
	 * \brief 動作終了時の処理
	 */
	virtual void OnFinished();

private:
	/**
	 * \brief stateの更新を実行
	 * \param dt
	 */
	void ExecuteStateUpdate(float dt);
	/**
	 * \brief 状態遷移の処理
	 */
	void HandleStateTransition();

private:
	//====================================================================*/
	//			private methods
	//====================================================================*/
	GimmickState currentState_ = GimmickState::Idle; //< 現在の状態
	GimmickState prevState_    = GimmickState::Idle; //< 前回の状態
};