#pragma once

/**
* \brief プレイヤー更新インターフェース
*/
struct IPlayerTickUpdater {
	virtual ~IPlayerTickUpdater() = default;
	virtual void Update(float dt) = 0;
};