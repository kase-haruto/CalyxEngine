#pragma once

/**
* \brief input に基づいてプレイヤーを更新するインターフェース
*/
struct IPlayerUpdater {
	virtual ~IPlayerUpdater() = default;
	virtual void Update(const struct PlayerCommand& cmd, float dt) = 0;
};