#pragma once
/* ======================================================================== */
/*	include space                                                            */
/* ======================================================================== */
#include <Engine/System/Event/EventBus.h>
#include <Game/3dObject/Actor/Enemy/Details/EnemyKind.h>

#include <queue>
#include <unordered_map>
#include <cstdint>


struct GainScore;

/*-----------------------------------------------------------------------------------------
 * ScoreService class
 * - スコア管理サービス
 *---------------------------------------------------------------------------------------*/
class ScoreService {
public:
	//===================================================================*/
	//			structs
	//===================================================================*/
	struct EnemyStat {
		int32_t count = 0;
		int32_t score = 0;
	};

private:
	struct Pending {
		int32_t amount;
	};

public:
	//===================================================================*/
	//			public methods
	//===================================================================*/
	ScoreService();
	~ScoreService();
	/**
	 * \brief 初期化
	 */
	void Initialize();

	/**
	 * \brief 終了処理
	 */
	void Shutdown();

	/**
	 * \brief 更新
	 */
	void Update();

	/**
	 * \brief スコアの合計を取得
	 */
	int32_t GetTotal() const { return total_; }

	/**
	 * \brief 敵撃破統計を取得（EnemyKind ベース）
	 */
	const std::unordered_map<EnemyKind, EnemyStat>& GetEnemyStats() const {
		return enemyStats_;
	}

private:
	//===================================================================*/
	//			private methods
	//===================================================================*/
	void OnGainScore(const GainScore& ev);

private:
	int32_t total_ = 0;						//< スコア合計
	std::queue<Pending> q_;					//< 遅延加算キュー
	
	std::unordered_map<EnemyKind, EnemyStat> enemyStats_;	//< 敵撃破統計
	EventBus::Connection connGainScore_;					//< GainScore イベントコネクション
};
