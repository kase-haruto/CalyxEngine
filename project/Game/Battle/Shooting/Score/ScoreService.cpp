#include "ScoreService.h"

/* ========================================================================
/*	include space
/* ===================================================================== */
#include <Engine/System/Event/EventBus.h>
#include "GainScore.h"

void ScoreService::Initialize(){
	EventBus::Subscribe<GainScore>(
		  [this](const GainScore& ev){ OnGainScore(ev); }
	  );
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