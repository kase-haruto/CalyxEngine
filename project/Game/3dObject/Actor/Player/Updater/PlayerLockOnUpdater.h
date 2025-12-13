#pragma once

#include <functional>
#include "IPlayerTickUpdater.h"
/**
 * \brief プレイヤーロックオン更新クラス
 */
class PlayerLockOnUpdater final
	:public IPlayerTickUpdater{
public:
	explicit PlayerLockOnUpdater(std::function<void(float)> updateFn);

	/**
	 * \brief 更新
	 * \param dt デルタタイム
	 */
	void Update(float dt)override ;

private:
	std::function<void(float)> update_;
};