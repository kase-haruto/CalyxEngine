#pragma once
#include "PatternAimedNWay.h"

// N-way の中心角を左右にスイープ
class PatternSweepFan
	: public PatternAimedNWay {
public:
	// 時間進行。periodSec に応じて位相を進め、親の centerDeg を更新する
	void Advance(float dt);
	void ResetPhase(float ph) { phase = ph; }

public:
	// 往復周期
	float periodSec = 2.5f;
	// スイープの振れ幅
	float amplitudeDeg = 36.0f;
	// 内部位相 [0, 2π)
	float phase = 0.0f;
};
