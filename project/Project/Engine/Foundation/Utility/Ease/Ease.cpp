#include "Ease.h"

// Linear
float CalyxEase::Linear(float t){
	return t;
}

// Quad
float CalyxEase::EaseInQuad(float t){
	return t * t;
}

float CalyxEase::EaseOutQuad(float t){
	return t * (2 - t);
}

float CalyxEase::EaseInOutQuad(float t){
	return t < 0.5f ? 2 * t * t : -1 + (4 - 2 * t) * t;
}

// Cubic
float CalyxEase::EaseInCubic(float t){
	return t * t * t;
}

float CalyxEase::EaseOutCubic(float t){
	return (--t) * t * t + 1;
}

float CalyxEase::EaseInOutCubic(float t){
	return t < 0.5f ? 4 * t * t * t : (t - 1) * (2 * t - 2) * (2 * t - 2) + 1;
}

// Sine
float CalyxEase::EaseInSine(float t){
	return 1 - std::cos((t * std::numbers::pi_v<float>) / 2);
}

float CalyxEase::EaseOutSine(float t){
	return std::sin((t * std::numbers::pi_v<float>) / 2);
}

float CalyxEase::EaseInOutSine(float t){
	return -(std::cos(std::numbers::pi_v<float> *t) - 1) / 2;
}

// Exponential
float CalyxEase::EaseInExpo(float t){
	return t == 0 ? 0 : std::powf(2, 10 * (t - 1));
}

float CalyxEase::EaseOutExpo(float t){
	return t == 1 ? 1 : 1 - std::powf(2, -10 * t);
}

float CalyxEase::EaseInOutExpo(float t){
	if (t == 0) return 0;
	if (t == 1) return 1;
	return t < 0.5f
		? std::powf(2, 10 * (2 * t - 1)) / 2
		: (2 - std::powf(2, -10 * (2 * t - 1))) / 2;
}

// Back
float CalyxEase::EaseInBack(float t, float s){
	return t * t * ((s + 1) * t - s);
}

float CalyxEase::EaseOutBack(float t, float s){
	return (--t) * t * ((s + 1) * t + s) + 1;
}

float CalyxEase::EaseInOutBack(float t, float s){
	t *= 2;
	s *= 1.525f;
	if (t < 1){
		return 0.5f * (t * t * ((s + 1) * t - s));
	} else{
		t -= 2;
		return 0.5f * (t * t * ((s + 1) * t + s) + 2);
	}
}

