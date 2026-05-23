#pragma once

#include <externals/nlohmann/json.hpp>

namespace CalyxEngine {

	struct ManipulatorSettings {
		bool  useSnap = false;
		float snapTranslate[3] = {1.0f, 1.0f, 1.0f};
		float snapRotate = 15.0f;
		float snapScale[3] = {0.1f, 0.1f, 0.1f};
	};

	inline void to_json(nlohmann::json& j, const ManipulatorSettings& settings) {
		j = nlohmann::json{
			{"useSnap", settings.useSnap},
			{"snapTranslate", {settings.snapTranslate[0], settings.snapTranslate[1], settings.snapTranslate[2]}},
			{"snapRotate", settings.snapRotate},
			{"snapScale", {settings.snapScale[0], settings.snapScale[1], settings.snapScale[2]}},
		};
	}

	inline void from_json(const nlohmann::json& j, ManipulatorSettings& settings) {
		settings.useSnap = j.value("useSnap", settings.useSnap);
		if(const auto snapTranslate = j.find("snapTranslate"); snapTranslate != j.end() && snapTranslate->is_array() && snapTranslate->size() >= 3) {
			settings.snapTranslate[0] = (*snapTranslate)[0].get<float>();
			settings.snapTranslate[1] = (*snapTranslate)[1].get<float>();
			settings.snapTranslate[2] = (*snapTranslate)[2].get<float>();
		}
		settings.snapRotate = j.value("snapRotate", settings.snapRotate);
		if(const auto snapScale = j.find("snapScale"); snapScale != j.end() && snapScale->is_array() && snapScale->size() >= 3) {
			settings.snapScale[0] = (*snapScale)[0].get<float>();
			settings.snapScale[1] = (*snapScale)[1].get<float>();
			settings.snapScale[2] = (*snapScale)[2].get<float>();
		}
	}

} // namespace CalyxEngine