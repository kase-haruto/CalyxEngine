#pragma once

#include <externals/nlohmann/json.hpp>
#include "Engine/Application/UI/EngineUI/ManipulatorSettings.h"

namespace CalyxEngine{

	/////////////////////////////////////////////////////////////////////////////////////////
	//		ランタイム
	/////////////////////////////////////////////////////////////////////////////////////////
	struct EditorSettings {
		bool fullscreenGameViewOnPlay = false;
		bool DebugCameraRotateInverse = false;
		bool isManipulatorSnap = false;
	};

	/////////////////////////////////////////////////////////////////////////////////////////
	//		エンジン全体の設定
	/////////////////////////////////////////////////////////////////////////////////////////
	struct EngineSettingsData {
		EditorSettings editor;
		ManipulatorSettings manipulator;
	};

	inline void to_json(nlohmann::json& j, const EditorSettings& settings) {
		j = nlohmann::json{
			{"fullscreenGameViewOnPlay", settings.fullscreenGameViewOnPlay},
			{"DebugCameraRotateInverse", settings.DebugCameraRotateInverse},
			{"isManipulatorSnap", settings.isManipulatorSnap},
		};
	}

	inline void from_json(const nlohmann::json& j, EditorSettings& settings) {
		settings.fullscreenGameViewOnPlay = j.value("fullscreenGameViewOnPlay", settings.fullscreenGameViewOnPlay);
		settings.DebugCameraRotateInverse = j.value("DebugCameraRotateInverse", settings.DebugCameraRotateInverse);
		settings.isManipulatorSnap = j.value("isManipulatorSnap", settings.isManipulatorSnap);
	}

	inline void to_json(nlohmann::json& j, const EngineSettingsData& data) {
		j = nlohmann::json{
			{"editor", data.editor},
			{"manipulator", data.manipulator},
		};
	}

	inline void from_json(const nlohmann::json& j, EngineSettingsData& data) {
		EngineSettingsData defaults{};
		if(const auto editor = j.find("editor"); editor != j.end() && editor->is_object()) {
			data.editor = editor->get<EditorSettings>();
		} else {
			data.editor = defaults.editor;
		}
		if(const auto manipulator = j.find("manipulator"); manipulator != j.end() && manipulator->is_object()) {
			data.manipulator = manipulator->get<ManipulatorSettings>();
		} else {
			data.manipulator = defaults.manipulator;
		}
	}

}