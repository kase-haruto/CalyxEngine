#pragma once
#include <random>
#include <type_traits>
#include <Engine/Foundation/Math/Vector3.h>

/* ========================================================================
/*		ランダム生成
/* ===================================================================== */
/**
 * @brief 乱数値およびランダムなベクトルを生成する機能を提供するクラスです。
 */
class Random{
public:
	Random(){}
	~Random(){}

	template <typename T>
	static T Generate(T min, T max){
		static_assert(
			std::is_integral<T>::value || std::is_floating_point<T>::value,
			"Only integral or floating-point types are supported."
			);

		static thread_local std::random_device randomDevice;
		static thread_local std::mt19937 randomEngine {randomDevice()};

		if constexpr (std::is_integral<T>::value){
			std::uniform_int_distribution<T> dist(min, max);
			return dist(randomEngine);
		} else{
			std::uniform_real_distribution<T> dist(min, max);
			return dist(randomEngine);
		}
	}

	static CalyxEngine::Vector3 GenerateVector3(float min, float max){
		return CalyxEngine::Vector3(
			Generate<float>(min, max),
			Generate<float>(min, max),
			Generate<float>(min, max)
		);
	}

	/**
	 * @brief Generates a normalized three-dimensional vector from random components.
	 * @return A randomly oriented unit vector.
	 */
	static CalyxEngine::Vector3 GenerateUnitVector3(){
		float x = Generate<float>(-1.0f, 1.0f);
		float y = Generate<float>(-1.0f, 1.0f);
		float z = Generate<float>(-1.0f, 1.0f);
		CalyxEngine::Vector3 v(x, y, z);

		// 正規化して単位ベクトルを返す
		return v.Normalize();
	}

	/**
	 * @brief Generates a three-dimensional vector within component-wise bounds.
	 * @param min Lower bound for each component.
	 * @param max Upper bound for each component.
	 * @return A vector whose components are generated within the supplied bounds.
	 */
	static CalyxEngine::Vector3 GenerateVector3(const CalyxEngine::Vector3& min, const CalyxEngine::Vector3& max){
		auto safeMinX = ( std::min ) (min.x, max.x);
		auto safeMaxX = (std::max)(min.x, max.x);

		auto safeMinY = (std::min)(min.y, max.y);
		auto safeMaxY = (std::max)(min.y, max.y);

		auto safeMinZ = (std::min)(min.z, max.z);
		auto safeMaxZ = (std::max)(min.z, max.z);

		return CalyxEngine::Vector3(
			Generate<float>(safeMinX, safeMaxX),
			Generate<float>(safeMinY, safeMaxY),
			Generate<float>(safeMinZ, safeMaxZ)
		);
	}

};
template<>
inline uint8_t Random::Generate<uint8_t>(uint8_t min, uint8_t max){
	std::random_device rd;
	std::mt19937 gen(rd());
	std::uniform_int_distribution<int> dist(min, max); // int で代用
	return static_cast< uint8_t >(dist(gen));
}
