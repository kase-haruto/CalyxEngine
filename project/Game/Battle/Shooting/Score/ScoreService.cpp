#include "ScoreService.h"

/* ========================================================================
/*	include space
/* ===================================================================== */
#include <Engine/System/Event/EventBus.h>
#include "GainScore.h"

ScoreService::ScoreService() = default;

ScoreService::~ScoreService() = default;

void ScoreService::Initialize(){
	connGainScore_ = EventBus::Subscribe<GainScore>(		//コネクションを受け取って保持
		[this](const GainScore& ev) { OnGainScore(ev); });
}

void ScoreService::Shutdown(){}

void ScoreService::OnGainScore(const GainScore& ev) {
	int add = static_cast<int>(ev.amount);
	q_.push(Pending{ add });
}

void ScoreService::Update() {
	while (!q_.empty()) {
		total_ += q_.front().amount;
		q_.pop();
	}
}

void ScoreService::AddRaw(int v) { q_.push(Pending{ v }); }