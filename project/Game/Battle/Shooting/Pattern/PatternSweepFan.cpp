#include "PatternSweepFan.h"
void PatternSweepFan::Advance(float dt) {
	if (periodSec <= 0.0f) return;
	const float twoPi = 6.28318530717958647692f;
	const float omega = twoPi / periodSec;
	phase += omega * dt;
	if (phase > twoPi) { phase -= twoPi; }
	// -amplitude ～ +amplitude に中心角をスイープ
	centerDeg = std::sin(phase) * amplitudeDeg;
}
