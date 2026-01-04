#pragma once
#include "Ease.h" // CalyxEase本体の実装ヘッダを含む

enum class EaseType{
	Linear = 0,
	EaseInQuad,
	EaseOutQuad,
	EaseInOutQuad,
	EaseInCubic,
	EaseOutCubic,
	EaseInOutCubic,
	EaseInSine,
	EaseOutSine,
	EaseInOutSine,
	EaseInExpo,
	EaseOutExpo,
	EaseInOutExpo,
	EaseInBack,
	EaseOutBack,
	EaseInOutBack,
	Count
};

static constexpr const char* EaseTypeNames[] = {
	"Linear",
	"EaseInQuad",
	"EaseOutQuad",
	"EaseInOutQuad",
	"EaseInCubic",
	"EaseOutCubic",
	"EaseInOutCubic",
	"EaseInSine",
	"EaseOutSine",
	"EaseInOutSine",
	"EaseInExpo",
	"EaseOutExpo",
	"EaseInOutExpo",
	"EaseInBack",
	"EaseOutBack",
	"EaseInOutBack"
};

inline float ApplyEase(EaseType type, float t){
	switch (type){
		case EaseType::Linear: return CalyxEase::Linear(t);
		case EaseType::EaseInQuad: return CalyxEase::EaseInQuad(t);
		case EaseType::EaseOutQuad: return CalyxEase::EaseOutQuad(t);
		case EaseType::EaseInOutQuad: return CalyxEase::EaseInOutQuad(t);
		case EaseType::EaseInCubic: return CalyxEase::EaseInCubic(t);
		case EaseType::EaseOutCubic: return CalyxEase::EaseOutCubic(t);
		case EaseType::EaseInOutCubic: return CalyxEase::EaseInOutCubic(t);
		case EaseType::EaseInSine: return CalyxEase::EaseInSine(t);
		case EaseType::EaseOutSine: return CalyxEase::EaseOutSine(t);
		case EaseType::EaseInOutSine: return CalyxEase::EaseInOutSine(t);
		case EaseType::EaseInExpo: return CalyxEase::EaseInExpo(t);
		case EaseType::EaseOutExpo: return CalyxEase::EaseOutExpo(t);
		case EaseType::EaseInOutExpo: return CalyxEase::EaseInOutExpo(t);
		case EaseType::EaseInBack: return CalyxEase::EaseInBack(t);
		case EaseType::EaseOutBack: return CalyxEase::EaseOutBack(t);
		case EaseType::EaseInOutBack: return CalyxEase::EaseInOutBack(t);
		default: return t;
	}
}
