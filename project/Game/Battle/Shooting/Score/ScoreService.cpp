#include "ScoreService.h"

#include "GainScore.h"

ScoreService::ScoreService() = default;
ScoreService::~ScoreService() = default;

void ScoreService::Initialize() {
	connGainScore_ =
		EventBus::Subscribe<GainScore>([this](const GainScore& ev) {
			OnGainScore(ev);
		});
}

void ScoreService::Shutdown() {}

void ScoreService::OnGainScore(const GainScore& ev) {
	// 合計スコア
	q_.push(Pending{ ev.amount });

	// 敵撃破内訳
	if (ev.reason == ScoreReason::EnemyKill) {
		for (const auto& t : ev.tag) {
			if (t.rfind("enemyType:", 0) == 0) {
				auto& stat = enemyStats_[t];
				stat.count += 1;
				stat.score += ev.amount;
			}
		}
	}
}

void ScoreService::Update() {
	// 保留中スコアを合計に反映
	while (!q_.empty()) {
		total_ += q_.front().amount;
		q_.pop();
	}
}
