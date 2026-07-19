#pragma once

#include <Engine/Foundation/Math/Vector3.h>
#include <Engine/Foundation/Math/Vector4.h>
#include <externals/nlohmann/json.hpp>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <vector>

namespace CalyxEngine {

	enum class CurveValueMode : uint8_t {
		Constant,
		Linear,
		Curve,
		RandomBetweenConstants,
		RandomBetweenCurves,
	};

	struct FloatCurveKey {
		float time = 0.0f;
		float value = 0.0f;
	};

	// 汎用のfloatカーブデータ。GPUリソースやParticle状態は管理しない。
	struct FloatCurve {
		CurveValueMode mode = CurveValueMode::Constant;
		float constant = 1.0f;
		float constantMin = 0.0f;
		float constantMax = 1.0f;
		std::vector<FloatCurveKey> keys{{0.0f, 0.0f}, {1.0f, 1.0f}};
		std::vector<FloatCurveKey> minKeys{{0.0f, 0.0f}, {1.0f, 0.0f}};
		std::vector<FloatCurveKey> maxKeys{{0.0f, 1.0f}, {1.0f, 1.0f}};

		float Evaluate(float time, float random01 = 0.0f) const {
			const float t = std::clamp(time, 0.0f, 1.0f);
			const float r = std::clamp(random01, 0.0f, 1.0f);
			switch(mode) {
			case CurveValueMode::Constant: return constant;
			case CurveValueMode::Linear:
			case CurveValueMode::Curve: return EvaluateKeys(keys, t, constant);
			case CurveValueMode::RandomBetweenConstants: return std::lerp(constantMin, constantMax, r);
			case CurveValueMode::RandomBetweenCurves:
				return std::lerp(EvaluateKeys(minKeys, t, constantMin), EvaluateKeys(maxKeys, t, constantMax), r);
			}
			return constant;
		}

		static float EvaluateKeys(const std::vector<FloatCurveKey>& source, float time, float fallback) {
			if(source.empty()) return fallback;
			if(source.size() == 1 || time <= source.front().time) return source.front().value;
			for(size_t i = 1; i < source.size(); ++i) {
				if(time <= source[i].time) {
					const float span = source[i].time - source[i - 1].time;
					if(std::abs(span) <= 0.000001f) return source[i].value;
					return std::lerp(source[i - 1].value, source[i].value, (time - source[i - 1].time) / span);
				}
			}
			return source.back().value;
		}
	};

	struct Vector3Curve {
		FloatCurve x;
		FloatCurve y;
		FloatCurve z;
		Vector3 Evaluate(float time, float random01) const {
			return {x.Evaluate(time, random01), y.Evaluate(time, random01), z.Evaluate(time, random01)};
		}
	};

	struct ColorGradientKey {
		float time = 0.0f;
		Vector4 color{1.0f, 1.0f, 1.0f, 1.0f};
	};

	// 色補間のみを管理する汎用Gradient。TextureやDescriptorは管理しない。
	struct ColorGradient {
		std::vector<ColorGradientKey> keys{{0.0f, {1, 1, 1, 1}}, {1.0f, {1, 1, 1, 1}}};
		Vector4 Evaluate(float time) const {
			if(keys.empty()) return {1, 1, 1, 1};
			const float t = std::clamp(time, 0.0f, 1.0f);
			if(keys.size() == 1 || t <= keys.front().time) return keys.front().color;
			for(size_t i = 1; i < keys.size(); ++i) {
				if(t <= keys[i].time) {
					const float span = keys[i].time - keys[i - 1].time;
					if(std::abs(span) <= 0.000001f) return keys[i].color;
					return Vector4::Lerp(keys[i - 1].color, keys[i].color, (t - keys[i - 1].time) / span);
				}
			}
			return keys.back().color;
		}
	};

	NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(FloatCurveKey, time, value)
	NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(FloatCurve, mode, constant, constantMin, constantMax, keys, minKeys, maxKeys)
	NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(Vector3Curve, x, y, z)
	NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(ColorGradientKey, time, color)
	NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(ColorGradient, keys)

	inline uint32_t HashParticleSeed(uint32_t seed) {
		seed ^= seed >> 16;
		seed *= 0x7feb352du;
		seed ^= seed >> 15;
		seed *= 0x846ca68bu;
		return seed ^ (seed >> 16);
	}

	inline float ParticleRandom01(uint32_t particleSeed, uint32_t stream) {
		return static_cast<float>(HashParticleSeed(particleSeed ^ (stream * 0x9e3779b9u)) & 0x00ffffffu) / 16777215.0f;
	}

} // namespace CalyxEngine
