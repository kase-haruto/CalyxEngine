#include "EngineSettings.h"

#include <Engine/Foundation/Json/JsonFileIO.h>
#include <externals/imgui/imgui.h>
#include <Engine/Application/UI/EngineUI/ManipulatorSettingsUI.h>

#include <algorithm>
#include <filesystem>
#include <functional>
#include <unordered_map>

namespace CalyxEngine {

	namespace {
		const char* GetCategoryLabel(EngineSettings::Category category) {
			switch(category) {
			case EngineSettings::Category::Editor:
				return "Editor";
			default:
				return "Unknown";
			}
		}
	}

	EngineSettings* EngineSettings::GetInstance() {
		static EngineSettings instance;
		return &instance;
	}

	void EngineSettings::Initialize() {
		if(!Load()) {
			Save();
		}
	}

	bool EngineSettings::Load() {
		nlohmann::json json;
		if(!JsonFileIO::Read(kSettingsPath, json)) {
			data_ = EngineSettingsData{};
			return false;
		}

		data_ = EngineSettingsData{};
		ApplyJson(json);
		return true;
	}

	bool EngineSettings::Save() const {
		std::error_code ec;
		std::filesystem::create_directories(std::filesystem::path(kSettingsPath).parent_path(), ec);
		if(ec) {
			return false;
		}
		return JsonFileIO::Write(kSettingsPath, ToJson());
	}

	void EngineSettings::OpenSettingsWindow() {
		showSettingsWindow_ = true;
		editingData_ = data_;
		editingInitialized_ = true;
	}

	void EngineSettings::RenderSettingsWindow() {
		if(!showSettingsWindow_) {
			return;
		}
		if(!editingInitialized_) {
			editingData_ = data_;
			editingInitialized_ = true;
		}

		ImGui::SetNextWindowSize(ImVec2(460.0f, 260.0f), ImGuiCond_FirstUseEver);
		if(!ImGui::Begin("Settings", &showSettingsWindow_)) {
			ImGui::End();
			return;
		}

		const float footerHeight = ImGui::GetFrameHeightWithSpacing() + ImGui::GetStyle().ItemSpacing.y;
		const ImVec2 contentSize = ImGui::GetContentRegionAvail();
		const float leftWidth = 160.0f;

		ImGui::BeginChild("SettingsCategories", ImVec2(leftWidth, contentSize.y - footerHeight), true);
		const Category categories[] = {
			Category::Editor,
		};
		for(const Category category : categories) {
			const bool selected = selectedCategory_ == category;
			if(ImGui::Selectable(GetCategoryLabel(category), selected)) {
				selectedCategory_ = category;
			}
		}
		ImGui::EndChild();

		ImGui::SameLine();

		ImGui::BeginChild("SettingsDetails", ImVec2(0.0f, contentSize.y - footerHeight), false);
		ImGui::TextUnformatted(GetCategoryLabel(selectedCategory_));
		ImGui::Separator();

		using RenderFn = std::function<void(EngineSettingsData&)>;
		static const std::unordered_map<Category, RenderFn> renderers = {
			{Category::Editor, [](EngineSettingsData& data) {
				ImGui::Checkbox("Fullscreen game view on play", &data.editor.fullscreenGameViewOnPlay);
				ImGui::Checkbox("Debug camera rotate inverse", &data.editor.DebugCameraRotateInverse);
				ManipulatorSettingsUI::Render(data.manipulator);
			}},
		};

		if(const auto it = renderers.find(selectedCategory_); it != renderers.end()) {
			it->second(editingData_);
		}
		ImGui::EndChild();

		ImGui::Separator();
		const float buttonWidth = 96.0f;
		const float spacing = ImGui::GetStyle().ItemSpacing.x;
		const float totalWidth = buttonWidth * 2.0f + spacing;
		ImGui::SetCursorPosX((std::max)(ImGui::GetCursorPosX(), ImGui::GetWindowContentRegionMax().x - totalWidth));

		if(ImGui::Button("Apply", ImVec2(buttonWidth, 0.0f))) {
			data_ = editingData_;
			Save();
			showSettingsWindow_ = false;
			editingInitialized_ = false;
			settingsApplied_ = true;
		}
		ImGui::SameLine();
		if(ImGui::Button("Cancel", ImVec2(buttonWidth, 0.0f))) {
			editingData_ = data_;
			showSettingsWindow_ = false;
			editingInitialized_ = false;
		}

		ImGui::End();
	}

	void EngineSettings::SetManipulatorSettings(const ManipulatorSettings& settings) {
		data_.manipulator = settings;
		Save();
	}

	nlohmann::json EngineSettings::ToJson() const {
		return data_;
	}

	void EngineSettings::ApplyJson(const nlohmann::json& json) {
		if(!json.is_object()) {
			data_ = EngineSettingsData{};
			return;
		}
		data_ = json.get<EngineSettingsData>();
	}

} // namespace CalyxEngine