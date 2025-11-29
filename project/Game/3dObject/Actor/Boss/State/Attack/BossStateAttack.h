#pragma once
#include "../Base/BaseBossState.h"

/**
 * \brief ボスの攻撃状態クラス
 */
class BossStateAttack final
	: public BaseBossState {
public:
	//===================================================================*/
	//					public methods
	//===================================================================*/
	BossStateAttack();
	~BossStateAttack() override;

	/**
	 * @brief 更新処理
	 * @param dt
	 */
	void Update(float dt) override;
	/**
	 * @brief 状態に入るときの処理
	 */
	void Enter() override;
	/**
	 * @brief GUI表示
	 */
	void ShowGui() override;

private:
	//===================================================================*/
	//					private methods
	//===================================================================*/
};
