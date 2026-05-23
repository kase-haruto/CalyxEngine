#pragma once
#include <cmath>
#include <numbers>

namespace CalyxEngine {
	// Linear
	 float Linear(float t);

	// Quad
	 float EaseInQuad(float t);
	 float EaseOutQuad(float t);
	 float EaseInOutQuad(float t);

	// Cubic
	 float EaseInCubic(float t);
	 float EaseOutCubic(float t);
	 float EaseInOutCubic(float t);

	// Sine
	 float EaseInSine(float t);
	 float EaseOutSine(float t);
	 float EaseInOutSine(float t);

	// Exponential
	 float EaseInExpo(float t);
	 float EaseOutExpo(float t);
	 float EaseInOutExpo(float t);

	// Back
	 float EaseInBack(float t, float s = 1.70158f);
	 float EaseOutBack(float t, float s = 1.70158f);
	 float EaseInOutBack(float t, float s = 1.70158f);
}; // namespace CalyxEngine
