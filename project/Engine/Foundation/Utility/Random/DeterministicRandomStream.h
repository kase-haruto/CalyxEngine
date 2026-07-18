#pragma once

#include <Engine/Foundation/Math/Vector3.h>

#include <algorithm>
#include <cstdint>
#include <limits>

namespace CalyxEngine {

	// Emitter単位の決定論的乱数列。グローバル乱数状態やGPUリソースは管理しない。
	class DeterministicRandomStream {
	public:
		explicit DeterministicRandomStream(uint32_t seed = 1u) { Reset(seed); }

		void Reset(uint32_t seed) { state_ = seed != 0 ? seed : 0x6d2b79f5u; }

		uint32_t NextUInt() {
			uint32_t value = state_;
			value ^= value << 13;
			value ^= value >> 17;
			value ^= value << 5;
			state_ = value;
			return value;
		}

		float NextFloat(float minValue = 0.0f, float maxValue = 1.0f) {
			if(minValue > maxValue) std::swap(minValue, maxValue);
			const float normalized = static_cast<float>(NextUInt() & 0x00ffffffu) / 16777215.0f;
			return minValue + (maxValue - minValue) * normalized;
		}

		Vector3 NextVector3(const Vector3& minValue, const Vector3& maxValue) {
			return {NextFloat(minValue.x, maxValue.x), NextFloat(minValue.y, maxValue.y), NextFloat(minValue.z, maxValue.z)};
		}

		Vector3 NextUnitVector3() {
			for(int attempt = 0; attempt < 8; ++attempt) {
				Vector3 value = NextVector3({-1, -1, -1}, {1, 1, 1});
				if(value.LengthSquared() > 0.000001f) return value.Normalize();
			}
			return Vector3::Up();
		}

	private:
		uint32_t state_ = 1u;
	};

} // namespace CalyxEngine
