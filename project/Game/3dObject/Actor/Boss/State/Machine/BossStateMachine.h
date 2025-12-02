#pragma once

// game
#include "../../Details/BossStateType"

// c++
#include "Game/3dObject/Actor/Boss/State/Base/BaseBossState.h"

#include <memory>
#include <vector>

class Boss;
// fwd
class BaseBossState;

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
	/**
	 * \brief 状態遷移要求の処理
	 */
	void HandleTransition(const BaseBossState::TransitionRequest& req);
	/**
	 * \brief 初期状態の設定
	 */
	void SetInitialState(BossStateType type);
	/**
	 * \brief GUI表示
	 */
	void ShowGui();

	// setter
	void SetOwner(Boss* owner);

	// getter
	BaseBossState* GetCurrentState() const;
	BossStateType GetCurrentStateType() const;

	/**
	* \brief 状態の変更変更時の処理を行う
	* \param nextType 次の状態の種類
	*/
	void ChangeState(BossStateType nextType,int16_t param = 0);

	/**
	* \brief 状態を上に積む
	* \param nextType 状態の種類
	*/
	void PushState(BossStateType nextType);

private:
	//===================================================================*/
	//                    private method
	//===================================================================*/


	/**
	 * \brief 状態の破棄
	 */
	void PopState();

private:
	//===================================================================*/
	//                    private members
	//===================================================================*/
	std::vector<std::unique_ptr<BaseBossState>> stack_;
	Boss*                                       owner_ = nullptr;
};