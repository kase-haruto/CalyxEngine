#include "PatternAimedNWay.h"

// 生成
void PatternAimedNWay::Generate([[maybe_unused]] const PatternInput& in, [[maybe_unused]] PatternOutput& out){}

// getter
int PatternAimedNWay::GetNWay() const{ return nWay_; }
float PatternAimedNWay::GetSpreadDeg() const{ return spreadDeg_; }
float PatternAimedNWay::CenterDeg() const{ return centerDeg_; }

// setter
void PatternAimedNWay::SetNWay(int value){ nWay_ = value; }
void PatternAimedNWay::SetSpreadDeg(float value){ spreadDeg_ = value; }
void PatternAimedNWay::SetCenterDeg(float value){ centerDeg_ = value; }