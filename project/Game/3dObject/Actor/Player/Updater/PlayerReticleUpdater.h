#pragma once
#include "Game/3dObject/Actor/Player/Input/PlayerCommand.h"
#include "Game/3dObject/Actor/Player/PlayerContext.h"
#include "IPlayerUpdater.h"

/**
 * \brief プレイヤーレティクル更新クラス
 */
class PlayerReticleUpdater final
	: public IPlayerUpdater {
public:
	//===================================================================*/
	//			public methods
	//===================================================================*/
	PlayerReticleUpdater(const PlayerReticleContext& ctx);
	~PlayerReticleUpdater() override = default;

	/**
	 * \brief 更新
	 * \param ctx
	 */
	void Update(const PlayerCommand& cmd, float dt) override;

private:
	//===================================================================*/
	//			private methods
	//===================================================================*/
	PlayerReticleContext ctx_;
};
