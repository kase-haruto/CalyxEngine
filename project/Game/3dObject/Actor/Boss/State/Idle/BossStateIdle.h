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

private:
	//===================================================================*/
	//                    private methods
	//===================================================================*/
};
