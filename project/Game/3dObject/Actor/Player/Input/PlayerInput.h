#pragma once
#include <vector>
#include "PlayerCommand.h"

/*-----------------------------------------------------------------------------------------
 * PlayerInput class
 * - プレイヤー入力を収集するクラス
 * - 入力結果をコマンドとしてまとめる
 *---------------------------------------------------------------------------------------*/
class PlayerInput {
public:
	std::vector<PlayerCommand> CollectCommands(float dt);
};
