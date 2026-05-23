#include "ImGuiLayoutSwitcher.h"
#include <externals/imgui/imgui.h>

namespace CalyxEngine {

	ImGuiLayoutSwitcher::ImGuiLayoutSwitcher(std::vector<LayoutEntry> presets,
											 std::string			  defaultPath)
		: presets_(std::move(presets)), currentIniPath_(defaultPath), autoSavePath_(std::move(defaultPath)) {
		// ImGuiの.iniファイルパスを設定 (常に固定の保存先を使用)
		ImGui::GetIO().IniFilename = autoSavePath_.c_str();

		// 起動時は autoSavePath_ から復元を試みるが、
		// もし初回起動などで無ければデフォルトレイアウトなどを読み込む検討も必要。
		// ここではシンプルに "現在の設定" としてロードする。
		ImGui::LoadIniSettingsFromDisk(autoSavePath_.c_str());
	}

	void ImGuiLayoutSwitcher::DrawMenu() {
		if(ImGui::BeginMenu("Layout")) {
			// プリセットレイアウトの選択
			for(const auto& preset : presets_) {
				bool selected = (preset.path == currentIniPath_);
				if(ImGui::MenuItem(preset.name.c_str(), nullptr, selected)) {
					Apply(preset.path);
				}
			}

			ImGui::Separator();

			// ディスクからリロード
			if(ImGui::MenuItem("Reload from disk")) {
				ImGui::LoadIniSettingsFromDisk(currentIniPath_.c_str());
			}

			// 現在のレイアウトを保存
			if(ImGui::MenuItem("Save current layout")) {
				ImGui::SaveIniSettingsToDisk(currentIniPath_.c_str());
			}

			ImGui::Separator();

			// 名前を付けて保存（簡易版）
			static char saveAsBuffer[512] = "Resources/Assets/Configs/Editor/Layout/custom.ini";
			ImGui::InputText("##SaveAsPath", saveAsBuffer, IM_ARRAYSIZE(saveAsBuffer));
			ImGui::SameLine();
			if(ImGui::Button("Save As...")) {
				ImGui::SaveIniSettingsToDisk(saveAsBuffer);
				// 新しい保存先を現在の自動保存先として使用
				Apply(saveAsBuffer);
			}

			ImGui::EndMenu();
		}
	}

	void ImGuiLayoutSwitcher::Apply(const std::string& iniPath) {
		pendingIniPath_	  = iniPath;
		hasPendingApply_ = true;
		currentIniPath_ = iniPath;
	}

	void ImGuiLayoutSwitcher::ApplyPending() {
		if(!hasPendingApply_) return;

		// DockSpace/TabBar の描画中に ini を読み替えると ImGui の assertion に当たるため、
		// LevelEditor::Update() の先頭からここを呼び、UI 描画前にだけロードする。
		ImGui::LoadIniSettingsFromDisk(pendingIniPath_.c_str());

		hasPendingApply_ = false;
		pendingIniPath_.clear();
	}

	void ImGuiLayoutSwitcher::Save(const std::string& iniPath) {
		ImGui::SaveIniSettingsToDisk(iniPath.c_str());
		currentIniPath_ = iniPath;
	}

} // namespace CalyxEngine
