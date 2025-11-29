#pragma once
#include "../Base/BaseBossState.h"

/**
 * \brief ボスの待機状態クラス
 */
class BossStateIdle final
	: public BaseBossState {
public:
	//===================================================================*/
	//                    public methods
	//===================================================================*/
	BossStateIdle();
	~BossStateIdle()override;

	/**
	 * @brief 更新処理
	 * @param dt
	 */
	void Update(float dt) override;
	/**
	 * @brief GUI表示
	 */
	void ShowGui() override;

	/**
	 * \brief 状態に入るときの処理
	 */
	void Enter() override;

private:
	//===================================================================*/
	//                    private methods
	//===================================================================*/
};
