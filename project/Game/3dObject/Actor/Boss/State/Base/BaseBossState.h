#pragma once

#include "../Details/BossStateType"

/**
 * \brief ボスの状態インターフェース
 */
class BaseBossState {
public:
	//===================================================================*/
	//                    structs
	//===================================================================*/
	struct TransitionRequest {
		enum class Type { None, Change, Push, Pop };

		Type op = Type::None;
		BossStateType nextType;
	};

public:
	//===================================================================*/
	//                    public methods
	//===================================================================*/
	BaseBossState();
	virtual ~BaseBossState();

	/**
	 * \brief 状態に入るときの処理
	 */
	virtual void Enter() {}
	/**
	 * \brief 状態から出るときの処理
	 */
	virtual void Exit() {}
	/**
	 * \brief 状態の更新
	 */
	virtual void Update(float dt) = 0;

	// accessor ==========================================================//
	const TransitionRequest& GetTransitionRequest() const;
	BossStateType GetStateType() const;
	void SetStatypeType(BossStateType type);
	
protected:
	//===================================================================*/
	//                    protected methods
	//===================================================================*/
	/**
	 * \brief 状態遷移の要求
	 * \param nextType 次の状態の種類
	 */
	void RequestChange(BossStateType next);
	/**
	 * \brief 状態を上に積む要求
	 * \param next 次の状態の種類
	 */
	void RequestPush(BossStateType next);
	/**
	 * \brief 状態の破棄要求
	 */
	void RequestPop();

private:
	//===================================================================*/
	//                    private methods
	//===================================================================*/
	TransitionRequest request_;
	BossStateType state_;
};
