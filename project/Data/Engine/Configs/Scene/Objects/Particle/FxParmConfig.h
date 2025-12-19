#pragma once

#include <Engine/Foundation/Math/Vector3.h>
#include <externals/nlohmann/json.hpp>

namespace CalyxEffect {

	// -------------------------
	// FxValueMode enum
	// -------------------------
	enum class FxValueMode {
		Constant,
		Random,
		RandomSphere
	};

	// JSON enum対応
	inline void to_json(nlohmann::json& j, const FxValueMode& mode) {
		j = static_cast<int>(mode);
	}
	inline void from_json(const nlohmann::json& j, FxValueMode& mode) {
		mode = static_cast<FxValueMode>(j.get<int>());
	}

	// -------------------------
	// テンプレート基底
	// -------------------------
	template <typename T>
	struct FxParamConfig {
		FxValueMode mode = FxValueMode::Constant;
		T			constant{};
		T			min{};
		T			max{};
	};

	// -------------------------
	// 型別特殊構造体
	// -------------------------
	struct FxFloatParamConfig : public FxParamConfig<float> {
		using FxParamConfig<float>::FxParamConfig;
		FxFloatParamConfig() {
			constant = 1.0f;
			min		 = 0.0f;
			max		 = 1.0f;
		}
		FxFloatParamConfig(const FxParamConfig<float>& base)
			: FxParamConfig<float>(base) {}
	};

	struct Vector3ParamConfig : public FxParamConfig<CalyxMath::Vector3> {
		using FxParamConfig<CalyxMath::Vector3>::FxParamConfig;
		Vector3ParamConfig() {
			constant = CalyxMath::Vector3(1.0f, 1.0f, 1.0f);
			min		 = CalyxMath::Vector3(0.0f, 0.0f, 0.0f);
			max		 = CalyxMath::Vector3(1.0f, 1.0f, 1.0f);
		}
		Vector3ParamConfig(const FxParamConfig<CalyxMath::Vector3>& base)
			: FxParamConfig<CalyxMath::Vector3>(base) {}
	};

	// -------------------------
	// JSON対応
	// -------------------------
	NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(FxFloatParamConfig,
									   mode,
									   constant,
									   min,
									   max)

	inline void to_json(nlohmann::json& j, const Vector3ParamConfig& p) {
		j = nlohmann::json{
			{"mode", p.mode},
			{"constant", p.constant},
			{"min", p.min},
			{"max", p.max}};
	}

	inline void from_json(const nlohmann::json& j, Vector3ParamConfig& p) {
		j.at("mode").get_to(p.mode);
		j.at("constant").get_to(p.constant);
		j.at("min").get_to(p.min);
		j.at("max").get_to(p.max);
	}

} // namespace CalyxEffect