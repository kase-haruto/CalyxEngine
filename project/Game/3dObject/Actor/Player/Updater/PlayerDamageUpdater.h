#pragma once
#include "Game/3dObject/Actor/Player/PlayerContext.h"
#include "IPlayerTickUpdater.h"
/**
 * \brief プレイヤーダメージ更新クラス
 */
class PlayerDamageUpdater final 
	: public IPlayerTickUpdater {
public:
	//===================================================================*/
	//			public methods
	//===================================================================*/
	explicit PlayerDamageUpdater(const PlayerDamageContext& ctx);
	~PlayerDamageUpdater()override;

	/**
	 * \brief ダメージを適用
	 */
	void ApplyDamage(int value)const;

private:
	//===================================================================*/
	//			private methods
	//===================================================================*/
	PlayerDamageContext ctx_;
};