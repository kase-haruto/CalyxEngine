#pragma once
/* ========================================================================
/*	include space
/* ===================================================================== */
#include <Engine/System/Event/EventBus.h>
#include <queue>
#include <string>

struct GainScore;

/*-----------------------------------------------------------------------------------------
 * ScoreService class
 * - スコア管理サービス
 *---------------------------------------------------------------------------------------*/
class ScoreService {
	//===================================================================*/
	//			structs
	//===================================================================*/
public:
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
	/** \brief コンストラクタ / デストラクタ*/
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
	 * \return スコアの合計
	 */
	int32_t GetTotal() const { return total_; }
	/**
	 * \brief 敵の統計情報を取得
	 * \return 敵の統計情報のマップ
	 */
	const std::unordered_map<std::string, EnemyStat>& GetEnemyStats() const {
		return enemyStats_;
	}

private:
	//===================================================================*/
	//			private methods
	//===================================================================*/
	/**
	 * \brief スコア獲得イベントの処理
	 * \param ev
	 */
	void OnGainScore(const GainScore& ev);

private:
	int32_t									   total_ = 0;		//< スコアの合計
	std::queue<Pending>						   q_;				//< 保留中のスコア獲得要求キュー
	std::unordered_map<std::string, EnemyStat> enemyStats_;		//< 敵の統計情報マップ
	EventBus::Connection					   connGainScore_;	//< スコア獲得イベントのコネクション
};