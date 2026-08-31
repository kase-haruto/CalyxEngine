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
			case EngineSettings::Category::Graphics:
				return "Graphics";
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
		// 初回起動時は既定値を保存し、次回以降の設定ファイル形式を確立する。
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

		// 読込前に既定値へ戻し、旧ファイルで欠落した項目を安全な値で補完する。
		data_ = EngineSettingsData{};
		ApplyJson(json);
		return true;
	}

	bool EngineSettings::Save() const {
		// ユーザー設定Directoryが未作成でも保存できるよう、親Directoryを先に生成する。
		std::error_code ec;
		std::filesystem::create_directories(std::filesystem::path(kSettingsPath).parent_path(), ec);
		if(ec) {
			return false;
		}
		return JsonFileIO::Write(kSettingsPath, ToJson());
	}

	void EngineSettings::OpenSettingsWindow() {
		// Cancel可能な編集用Copyを作り、操作途中の値をRuntime設定へ直接反映しない。
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

		// Category一覧、詳細、確定Buttonを固定領域に分け、項目追加時もFooterを隠さない。
		const float footerHeight = ImGui::GetFrameHeightWithSpacing() + ImGui::GetStyle().ItemSpacing.y;
		const ImVec2 contentSize = ImGui::GetContentRegionAvail();
		const float leftWidth = 160.0f;

		ImGui::BeginChild("SettingsCategories", ImVec2(leftWidth, contentSize.y - footerHeight), true);
		const Category categories[] = {
			Category::Editor,
			Category::Graphics,
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

		// Categoryごとの描画処理をMapへ分離し、switchの肥大化を避ける。
		using RenderFn = std::function<void(EngineSettingsData&)>;
		static const std::unordered_map<Category, RenderFn> renderers = {
			{Category::Editor, [](EngineSettingsData& data) {
				ImGui::Checkbox("Fullscreen game view on play", &data.editor.fullscreenGameViewOnPlay);
				ImGui::Checkbox("Debug camera rotate inverse", &data.editor.DebugCameraRotateInverse);
				ImGui::Checkbox("Debug line rendering", &data.editor.renderDebugLinesInViewports);
				ImGui::Checkbox("Editor grid", &data.editor.showEditorGrid);
				ManipulatorSettingsUI::Render(data.manipulator);
			}},
			{Category::Graphics, [](EngineSettingsData& data) {
				ImGui::Checkbox("Point light shadows", &data.graphics.enablePointLightShadows);
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
			// Apply時だけ編集Copyを正式設定へ反映し、利用側へ変更通知Flagを立てる。
			data_ = editingData_;
			Save();
			showSettingsWindow_ = false;
			editingInitialized_ = false;
			settingsApplied_ = true;
		}
		ImGui::SameLine();
		if(ImGui::Button("Cancel", ImVec2(buttonWidth, 0.0f))) {
			// Cancelでは保存済み設定へ戻し、編集中の一時値を破棄する。
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

	void EngineSettings::SetEditorSettings(const EditorSettings& settings) {
		data_.editor = settings;
		Save();
	}

	nlohmann::json EngineSettings::ToJson() const {
		return data_;
	}

	void EngineSettings::ApplyJson(const nlohmann::json& json) {
		// Root型が不正な場合は例外的な部分読込を行わず、全設定を既定値へ戻す。
		if(!json.is_object()) {
			data_ = EngineSettingsData{};
			return;
		}
		data_ = json.get<EngineSettingsData>();
	}

} // namespace CalyxEngine
