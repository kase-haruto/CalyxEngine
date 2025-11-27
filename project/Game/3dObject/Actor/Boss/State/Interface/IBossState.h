#pragma once

#include "../Details/BossStateType"

/**
 * \brief ボスの状態インターフェース
 */
class IBossState {
public:
	//===================================================================*/
	//                    structs
	//===================================================================*/
	struct TransitionRequest {
		bool		  hasRequest = false;
		BossStateType nextType;
	};

public:
	//===================================================================*/
	//                    public methods
	//===================================================================*/
	IBossState();
	virtual ~IBossState();

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
protected:
	//===================================================================*/
	//                    protected methods
	//===================================================================*/
	/**
	 * \brief 状態遷移の要求
	 * \param nextType 次の状態の種類
	 */
	void RequestTransition(BossStateType nextType);

private:
	//===================================================================*/
	//                    private methods
	//===================================================================*/
	TransitionRequest request_;
	BossStateType state_;
};
