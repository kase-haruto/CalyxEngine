#pragma once

#include "IShootPattern.h"

class PatternAimedNWay :
	public IShootPattern {
public:
	//===================================================================*/
	//					 public methods
	//===================================================================*/
	void Generate(const PatternInput& in, PatternOutput& out) override ;

	//--------- accessor -------------------------------------------------
	//getter
	int GetNWay()const;
	float GetSpreadDeg()const;
	float CenterDeg()const;

	//setter
	void SetNWay(int value);
	void SetSpreadDeg(float value);
	void SetCenterDeg(float value);

private:
	//===================================================================*/
	//					 private methods
	//===================================================================*/
	int nWay_ = 7;
	float spreadDeg_ = 30.0f; // 総角度
	float centerDeg_ = 0.0f;  // 中心オフセット（スイープ用）
};