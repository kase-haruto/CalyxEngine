#pragma once

#include <Engine/Foundation/Math/Vector3.h>
#include <externals/nlohmann/json.hpp>

namespace CalyxEngine {

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
	/**
	 * @brief FxParamConfigに関するデータを保持する構造体です。
	 */
	struct FxParamConfig {
		FxValueMode mode = FxValueMode::Constant;
		T			constant{};
		T			min{};
		T			max{};
	};

	// -------------------------
	// 型別特殊構造体
	// -------------------------
	/**
	 * @brief FxFloatParamConfigに関するデータを保持する構造体です。
	 */
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

	/**
	 * @brief Vector3ParamConfigに関するデータを保持する構造体です。
	 */
	struct Vector3ParamConfig : public FxParamConfig<CalyxEngine::Vector3> {
		using FxParamConfig<CalyxEngine::Vector3>::FxParamConfig;
		Vector3ParamConfig() {
			constant = CalyxEngine::Vector3(1.0f, 1.0f, 1.0f);
			min		 = CalyxEngine::Vector3(0.0f, 0.0f, 0.0f);
			max		 = CalyxEngine::Vector3(1.0f, 1.0f, 1.0f);
		}
		Vector3ParamConfig(const FxParamConfig<CalyxEngine::Vector3>& base)
			: FxParamConfig<CalyxEngine::Vector3>(base) {}
	};

	// -------------------------
	// JSON対応
	// -------------------------
	inline void to_json(nlohmann::json& j, const FxFloatParamConfig& p) {
		j = nlohmann::json{
			{"mode", p.mode},
			{"constant", p.constant},
			{"min", p.min},
			{"max", p.max}};
	}

	inline void from_json(const nlohmann::json& j, FxFloatParamConfig& p) {
		if(j.is_number()) {
			p.mode = FxValueMode::Constant;
			p.constant = j.get<float>();
			p.min = p.constant;
			p.max = p.constant;
			return;
		}

		if(!j.is_object()) return;

		if(auto it = j.find("mode"); it != j.end() && !it->is_null()) {
			p.mode = it->get<FxValueMode>();
		}
		if(auto it = j.find("constant"); it != j.end() && !it->is_null()) {
			p.constant = it->get<float>();
		}
		if(auto it = j.find("min"); it != j.end() && !it->is_null()) {
			p.min = it->get<float>();
		}
		if(auto it = j.find("max"); it != j.end() && !it->is_null()) {
			p.max = it->get<float>();
		}
	}

	inline void to_json(nlohmann::json& j, const Vector3ParamConfig& p) {
		j = nlohmann::json{
			{"mode", p.mode},
			{"constant", p.constant},
			{"min", p.min},
			{"max", p.max}};
	}

	inline void from_json(const nlohmann::json& j, Vector3ParamConfig& p) {
		if(j.is_number()) {
			const float value = j.get<float>();
			p.mode = FxValueMode::Constant;
			p.constant = CalyxEngine::Vector3(value,value,value);
			p.min = p.constant;
			p.max = p.constant;
			return;
		}

		if(j.is_array()) {
			p.mode = FxValueMode::Constant;
			p.constant = j.get<CalyxEngine::Vector3>();
			p.min = p.constant;
			p.max = p.constant;
			return;
		}

		if(!j.is_object()) return;

		if(auto it = j.find("mode"); it != j.end() && !it->is_null()) {
			p.mode = it->get<FxValueMode>();
		}
		if(auto it = j.find("constant"); it != j.end() && !it->is_null()) {
			if(it->is_number()) {
				const float value = it->get<float>();
				p.constant = CalyxEngine::Vector3(value,value,value);
			} else {
				p.constant = it->get<CalyxEngine::Vector3>();
			}
		}
		if(auto it = j.find("min"); it != j.end() && !it->is_null()) {
			if(it->is_number()) {
				const float value = it->get<float>();
				p.min = CalyxEngine::Vector3(value,value,value);
			} else {
				p.min = it->get<CalyxEngine::Vector3>();
			}
		}
		if(auto it = j.find("max"); it != j.end() && !it->is_null()) {
			if(it->is_number()) {
				const float value = it->get<float>();
				p.max = CalyxEngine::Vector3(value,value,value);
			} else {
				p.max = it->get<CalyxEngine::Vector3>();
			}
		}
	}

} // namespace CalyxEngine
