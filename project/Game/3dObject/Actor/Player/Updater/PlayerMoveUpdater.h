#pragma once
#include "../Input/PlayerCommand.h"
#include "../PlayerContext.h"
#include "IPlayerUpdater.h"

/**
 * \brief プレイヤー移動更新クラス
 */
class PlayerMoveUpdater final
	: public IPlayerUpdater {
public:
	//===================================================================*/
	//			public methods
	//===================================================================*/
	explicit PlayerMoveUpdater(const PlayerMoveContext& ctx);
	~PlayerMoveUpdater() override;
	/**
	 * \brief 実行
	 */
	void Update(const PlayerCommand& cmd, float dt)override;

private:
	//===================================================================*/
	//			private methods
	//===================================================================*/
	PlayerMoveContext ctx_;
};