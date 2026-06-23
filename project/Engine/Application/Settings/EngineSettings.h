#pragma once

#include "EngineSettingsDetails.h"
#include <externals/nlohmann/json.hpp>

namespace CalyxEngine {

	class EngineSettings {
	public:
		enum class Category {
			Editor,
			Graphics,
		};

		static EngineSettings* GetInstance();

		void Initialize();
		bool Load();
		bool Save() const;

		void OpenSettingsWindow();
		void RenderSettingsWindow();

		const EngineSettingsData& GetData() const { return data_; }
		bool ConsumeApplied() {
			if(!settingsApplied_) return false;
			settingsApplied_ = false;
			return true;
		}

		void SetManipulatorSettings(const ManipulatorSettings& settings);
		void SetEditorSettings(const EditorSettings& settings);

	private:
		EngineSettings() = default;

		static constexpr const char* kSettingsPath = "generated/Settings/EngineSettings.json";

		nlohmann::json ToJson() const;
		void		   ApplyJson(const nlohmann::json& json);

		EngineSettingsData data_{};
		EngineSettingsData editingData_{};
		Category		   selectedCategory_ = Category::Editor;
		bool			   showSettingsWindow_ = false;
		bool			   editingInitialized_ = false;
		bool settingsApplied_ = false;
	};

} // namespace CalyxEngine
