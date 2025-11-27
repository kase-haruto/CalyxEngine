#pragma once

// game
#include "../Details/BossStateType"

// c++
#include <memory>

// fwd
class IBossState;

/**
 * ボスの状態の切り替えと更新などを担うクラス
 */
class BossStateMachine {
public:
	//===================================================================*/
	//                    public methods
	//===================================================================*/
	BossStateMachine();
	~BossStateMachine();

	/**
	 * \brief 状態の更新
	 */
	void Update(float dt);

private:
	//===================================================================*/
	//                    private method
	//===================================================================*/
	void ChangeState(BossStateType nextType);
	
private:
	//===================================================================*/
	//                    private members
	//===================================================================*/
	std::unique_ptr<IBossState> currentState_ = nullptr;
};
