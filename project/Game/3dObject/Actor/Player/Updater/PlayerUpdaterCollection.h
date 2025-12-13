#pragma once
#include <memory>
#include <vector>

#include "IPlayerUpdater.h"
#include "IPlayerTickUpdater.h"
#include "Game/3dObject/Actor/Player/Input/PlayerCommand.h"

/**
 * \brief プレイヤー更新群管理クラス
 */
class PlayerUpdaterCollection {
public:
	//=====================================================================
	// Public Methods
	//=====================================================================
	// 登録
	void AddCommandUpdater(std::unique_ptr<IPlayerUpdater> updater);
	void AddTickUpdater(std::unique_ptr<IPlayerTickUpdater> updater);
	/**
	 * \brief コマンドに基づく更新
	 */
	void CommandUpdate(const std::vector<PlayerCommand>& commands, float dt)const;
	/**
	 * \brief 毎フレーム更新
	 */
	void TickUpdate(float dt)const;
	/**
	 * \brief クリア
	 */
	void Clear();

private:
	//=====================================================================
	// Private Variables
	//=====================================================================
	std::vector<std::unique_ptr<IPlayerUpdater>>     commandUpdaters_;
	std::vector<std::unique_ptr<IPlayerTickUpdater>> tickUpdaters_;
};