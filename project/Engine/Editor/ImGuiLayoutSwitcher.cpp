#include "ImGuiLayoutSwitcher.h"
#include <externals/imgui/imgui.h>

namespace CalyxEditor {

	ImGuiLayoutSwitcher::ImGuiLayoutSwitcher(std::vector<LayoutEntry> presets,
	                                         std::string defaultPath)
		: presets_(std::move(presets)), currentIniPath_(std::move(defaultPath)) {
		// ImGuiの.iniファイルパスを設定
		ImGui::GetIO().IniFilename = currentIniPath_.c_str();
		// 起動時に現在のレイアウトをロード
		ImGui::LoadIniSettingsFromDisk(currentIniPath_.c_str());
	}

	void ImGuiLayoutSwitcher::DrawMenu() {
		if (ImGui::BeginMenu("Layout")) {
			// プリセットレイアウトの選択
			for (const auto& preset : presets_) {
				bool selected = (preset.path == currentIniPath_);
				if (ImGui::MenuItem(preset.name.c_str(), nullptr, selected)) {
					Apply(preset.path);
				}
			}

			ImGui::Separator();

			// ディスクからリロード
			if (ImGui::MenuItem("Reload from disk")) {
				ImGui::LoadIniSettingsFromDisk(currentIniPath_.c_str());
			}

			// 現在のレイアウトを保存
			if (ImGui::MenuItem("Save current layout")) {
				ImGui::SaveIniSettingsToDisk(currentIniPath_.c_str());
			}

			ImGui::Separator();

			// 名前を付けて保存（簡易版）
			static char saveAsBuffer[512] = "Resources/Assets/Configs/Editor/Layout/custom.ini";
			ImGui::InputText("##SaveAsPath", saveAsBuffer, IM_ARRAYSIZE(saveAsBuffer));
			ImGui::SameLine();
			if (ImGui::Button("Save As...")) {
				ImGui::SaveIniSettingsToDisk(saveAsBuffer);
				// 新しい保存先を現在の自動保存先として使用
				Apply(saveAsBuffer);
			}

			ImGui::EndMenu();
		}
	}

	void ImGuiLayoutSwitcher::Apply(const std::string& iniPath) {
		// 新しいレイアウトをロードし、自動保存先も切り替え
		ImGui::LoadIniSettingsFromDisk(iniPath.c_str());
		currentIniPath_ = iniPath;
		ImGui::GetIO().IniFilename = currentIniPath_.c_str();
	}

} // namespace CalyxEditor
