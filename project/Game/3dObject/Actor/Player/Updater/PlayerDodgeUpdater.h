#pragma once
#include "IPlayerUpdater.h"
#include "Game/3dObject/Actor/Player/PlayerContext.h"

/**
 * \brief プレイヤー回避アップデーター
 */
class PlayerDodgeUpdater final
	: public IPlayerUpdater {
public:
	//=================================================================*/
	//			public methods
	//=================================================================*/
	explicit PlayerDodgeUpdater(const PlayerCombatContext& ctx);

	/**
	 * \brief 更新
	 * \param cmd	コマンド
	 * \param dt	デルタタイム
	 */
	void Update(const struct PlayerCommand& cmd, float dt) override;

private:
	//=================================================================*/
	//			private members
	//=================================================================*/
	PlayerCombatContext ctx_;
};
