#pragma once

#include "IShootPattern.h"

class PatternCircleRing
	: public IShootPattern {
public:
	void Generate(const PatternInput& in, PatternOutput& out) override;

public:
	int count = 24;
};