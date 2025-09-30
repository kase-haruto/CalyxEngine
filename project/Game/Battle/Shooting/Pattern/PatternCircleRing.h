#pragma once

#include "IShootPattern.h"

class PatternCircleRing
	: public IShootPattern {
public:
	// リングの弾数
	int count = 24;

	void Generate(const PatternInput& in, PatternOutput& out) override;
};