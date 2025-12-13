#pragma once
#include "Game/3dObject/Actor/Player/PlayerContext.h"
#include "IPlayerUpdater.h"

class PlayerCombatUpdater final
	: public IPlayerUpdater {
public:
	//=====================================================================*/
	// Public Methods
	//=====================================================================*/
	explicit PlayerCombatUpdater(const PlayerCombatContext& ctx);
	/**
	 * \brief 更新
	 * \param cmd		プレイヤーコマンド
	 * \param dt		デルタタイム
	 */
	void Update(const struct PlayerCommand& cmd, float dt) override;

private:
	//=====================================================================*/
	// Private Variables
	//=====================================================================*/
	PlayerCombatContext ctx_;
};
