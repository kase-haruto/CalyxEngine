#pragma once
#include <memory>
#include "IShootPattern.h"
// 既存のパターン
#include "PatternCircleRing.h"
#include "PatternSweepFan.h"
#include "PatternSpiral.h"
#include "PatternAimedNWay.h"

enum class BulletPatternKind : int{
	CircleRing = 0,
	SweepFan = 1,
	Spiral = 2,
	AimedNWay = 3,
};

inline const char* ToString(BulletPatternKind k){
	switch (k){
		case BulletPatternKind::CircleRing: return "CircleRing";
		case BulletPatternKind::SweepFan:   return "SweepFan";
		case BulletPatternKind::Spiral:     return "Spiral";
		case BulletPatternKind::AimedNWay:  return "AimedNWay";
		default: return "?";
	}
}

inline std::unique_ptr<IShootPattern> CreatePattern(BulletPatternKind kind){
	switch (kind){
		case BulletPatternKind::CircleRing:
		{
			auto p = std::make_unique<PatternCircleRing>();
			p->count = 24; // デフォルト
			return p;
		}
		case BulletPatternKind::SweepFan:
		{
			auto p = std::make_unique<PatternSweepFan>();
			return p;
		}
		case BulletPatternKind::Spiral:
		{
			auto p = std::make_unique<PatternSpiral>();
			return p;
		}
		case BulletPatternKind::AimedNWay:
		{
			auto p = std::make_unique< PatternAimedNWay>();
			return p;
		}

	}
	return nullptr;
}
