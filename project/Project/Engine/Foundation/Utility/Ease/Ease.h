#pragma once
#include <cmath>
#include <numbers>

class CalyxEase {
  public:
	// Linear
	static float Linear(float t);

	// Quad
	static float EaseInQuad(float t);
	static float EaseOutQuad(float t);
	static float EaseInOutQuad(float t);

	// Cubic
	static float EaseInCubic(float t);
	static float EaseOutCubic(float t);
	static float EaseInOutCubic(float t);

	// Sine
	static float EaseInSine(float t);
	static float EaseOutSine(float t);
	static float EaseInOutSine(float t);

	// Exponential
	static float EaseInExpo(float t);
	static float EaseOutExpo(float t);
	static float EaseInOutExpo(float t);

	// Back
	static float EaseInBack(float t, float s = 1.70158f);
	static float EaseOutBack(float t, float s = 1.70158f);
	static float EaseInOutBack(float t, float s = 1.70158f);
};
