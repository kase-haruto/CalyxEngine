#pragma once
#include "PlayerContext.h"

class Player;

/*-----------------------------------------------------------------------------------------
 * PlayerContextBuilder class
 * - プレイヤーの各種コンテキストを構築するクラス
 * - アクション用/状態用コンテキストを生成する
 *---------------------------------------------------------------------------------------*/
class PlayerContextBuilder {
public:
	explicit PlayerContextBuilder(Player& player);

	PlayerActionContext BuildAction();
	PlayerStateContext  BuildState();

private:
	Player& player_;
};
