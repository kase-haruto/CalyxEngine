#pragma once

#include "IShootPattern.h"

class PatternAimedNWay :
	public IShootPattern {
public:
	//===================================================================*/
	//					 public methods
	//===================================================================*/
	void Generate(const PatternInput& in, PatternOutput& out) override ;

public:
	//===================================================================*/
	//					 private methods
	//===================================================================*/
	int nWay = 3;
	float spreadDeg = 30.0f; // 総角度
	float centerDeg = 0.0f;  // 中心オフセット（スイープ用）
};