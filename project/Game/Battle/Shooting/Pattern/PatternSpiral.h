#pragma once

#include "IShootPattern.h"

class PatternSpiral
	: public IShootPattern {
public:
	//===================================================================*/
	//					 public methods
	//===================================================================*/
	void Advance(float dt);
	void Generate(const PatternInput& in, PatternOutput& out) override;
	
public:
	//===================================================================*/
	//					 public methods
	//===================================================================*/
	float angularSpeedDegPerSec = 360.0f;
	float tipDeg = 15.0f;
	float angleDeg = 0.0f;
};
