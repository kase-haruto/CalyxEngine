#include "ImGuiLayoutSwitcher.h"

#include <CalyxEngine/Project.h>
#include <externals/imgui/imgui.h>
#include <externals/imgui/imgui_internal.h>

#include <cstdio>
#include <filesystem>

namespace CalyxEngine {

	////////////////////////////////////////////////////////////////////////////////////////////
	//		コンストラクタ
	////////////////////////////////////////////////////////////////////////////////////////////
	ImGuiLayoutSwitcher::ImGuiLayoutSwitcher(std::vector<LayoutEntry> presets,
											 std::string			  defaultPath)
		: presets_(std::move(presets)), currentIniPath_(std::move(defaultPath)), autoSavePath_(currentIniPath_) {
		// ImGui の自動保存先を初期レイアウトに設定する
		ImGui::GetIO().IniFilename = autoSavePath_.c_str();

		// 起動時の通常復元は ImGui の既定挙動に近い形で読み込む
		ImGui::LoadIniSettingsFromDisk(autoSavePath_.c_str());
	}

	////////////////////////////////////////////////////////////////////////////////////////////
	//		レイアウトメニュー描画
	////////////////////////////////////////////////////////////////////////////////////////////
	void ImGuiLayoutSwitcher::DrawMenu() {
		if(ImGui::BeginMenu("Layout")) {
			// 登録済みプリセットをメニューに列挙する
			for(const auto& preset : presets_) {
				const std::string presetPath = ResolveLayoutPath(preset.path);
				const bool		  selected	 = (presetPath == currentIniPath_);
				if(ImGui::MenuItem(preset.name.c_str(), nullptr, selected)) {
					Apply(preset.path);
				}
			}

			ImGui::Separator();

			// 現在の ini を専用レイアウトとして読み直す
			if(ImGui::MenuItem("Reload from disk")) {
				ReloadCurrent();
			}

			// 現在のレイアウトを現在の ini へ保存する
			if(ImGui::MenuItem("Save current layout")) {
				SaveCurrent();
			}

			ImGui::Separator();

			// 名前を付けて保存するためのパス入力欄を初期化する
			static char saveAsBuffer[512] = "";
			static bool saveAsPathInitialized = false;
			if(!saveAsPathInitialized) {
				const std::string defaultSavePath =
					Calyx::ResolveAssetPath("Configs/Editor/Layout/custom.ini").generic_string();
				std::snprintf(saveAsBuffer, sizeof(saveAsBuffer), "%s", defaultSavePath.c_str());
				saveAsPathInitialized = true;
			}

			// 入力されたパスへ保存し、そのファイルを現在レイアウトとして扱う
			ImGui::InputText("##SaveAsPath", saveAsBuffer, IM_ARRAYSIZE(saveAsBuffer));
			ImGui::SameLine();
			if(ImGui::Button("Save As...")) {
				Save(saveAsBuffer);
				Apply(saveAsBuffer);
			}

			ImGui::EndMenu();
		}
	}

	////////////////////////////////////////////////////////////////////////////////////////////
	//		レイアウト適用予約
	////////////////////////////////////////////////////////////////////////////////////////////
	void ImGuiLayoutSwitcher::Apply(const std::string& iniPath) {
		// 呼び出し側がファイル名だけを指定しても Layout ディレクトリへ解決する
		const std::string resolvedPath = ResolveLayoutPath(iniPath);
		if(resolvedPath.empty()) return;

		// UI 描画中の ini 読み替えを避けるため、次の Update まで予約する
		saveBeforeApplyPath_ = currentIniPath_;
		pendingIniPath_		 = resolvedPath;
		currentIniPath_		 = resolvedPath;
		hasPendingApply_	 = true;
	}

	////////////////////////////////////////////////////////////////////////////////////////////
	//		予約レイアウト適用
	////////////////////////////////////////////////////////////////////////////////////////////
	void ImGuiLayoutSwitcher::ApplyPending() {
		if(!hasPendingApply_) return;

		// 切り替え前のレイアウトを元の ini に保存する
		if(!saveBeforeApplyPath_.empty() && saveBeforeApplyPath_ != pendingIniPath_) {
			SaveToDisk(saveBeforeApplyPath_);
		}

		// 切り替え先 ini だけを専用レイアウトとして読み込む
		LoadExclusiveFromDisk(pendingIniPath_);

		// 今後の自動保存先を切り替え先 ini に更新する
		autoSavePath_ = pendingIniPath_;
		ImGui::GetIO().IniFilename = autoSavePath_.c_str();

		// 適用予約の状態をクリアする
		hasPendingApply_ = false;
		pendingIniPath_.clear();
		saveBeforeApplyPath_.clear();
	}

	////////////////////////////////////////////////////////////////////////////////////////////
	//		指定レイアウト保存
	////////////////////////////////////////////////////////////////////////////////////////////
	void ImGuiLayoutSwitcher::Save(const std::string& iniPath) {
		// 保存先パスを解決してから現在レイアウトとして採用する
		const std::string resolvedPath = ResolveLayoutPath(iniPath);
		if(resolvedPath.empty()) return;

		// 現在の ImGui レイアウトを保存する
		SaveToDisk(resolvedPath);
		currentIniPath_ = resolvedPath;
	}

	////////////////////////////////////////////////////////////////////////////////////////////
	//		現在レイアウト保存
	////////////////////////////////////////////////////////////////////////////////////////////
	void ImGuiLayoutSwitcher::SaveCurrent() {
		// 現在選択中の ini へ保存する
		SaveToDisk(currentIniPath_);
	}

	////////////////////////////////////////////////////////////////////////////////////////////
	//		現在レイアウト再読み込み
	////////////////////////////////////////////////////////////////////////////////////////////
	void ImGuiLayoutSwitcher::ReloadCurrent() {
		// 現在パスを解決し直し、専用レイアウトとして読み込む
		const std::string resolvedPath = ResolveLayoutPath(currentIniPath_);
		if(resolvedPath.empty()) return;

		LoadExclusiveFromDisk(resolvedPath);
		currentIniPath_ = resolvedPath;
	}

	////////////////////////////////////////////////////////////////////////////////////////////
	//		レイアウトパス解決
	////////////////////////////////////////////////////////////////////////////////////////////
	std::string ImGuiLayoutSwitcher::ResolveLayoutPath(std::string_view path) const {
		if(path.empty()) return {};

		// 絶対パスはそのまま正規化して使う
		std::filesystem::path layoutPath(path);
		if(layoutPath.is_absolute()) {
			return layoutPath.lexically_normal().generic_string();
		}

		// ディレクトリを含む相対パスは呼び出し側の指定を尊重する
		if(layoutPath.has_parent_path()) {
			return layoutPath.lexically_normal().generic_string();
		}

		// 初期自動保存ファイルだけは従来通り作業ディレクトリ直下として扱う
		if(path == autoSavePath_) {
			return layoutPath.lexically_normal().generic_string();
		}

		// ファイル名のみの場合はプロジェクトの Layout ディレクトリへ解決する
		return (Calyx::ResolveAssetPath("Configs/Editor/Layout") / layoutPath)
			.lexically_normal()
			.generic_string();
	}

	////////////////////////////////////////////////////////////////////////////////////////////
	//		レイアウト保存
	////////////////////////////////////////////////////////////////////////////////////////////
	void ImGuiLayoutSwitcher::SaveToDisk(const std::string& iniPath) const {
		if(iniPath.empty()) return;

		// 保存先ディレクトリが存在しない場合は作成する
		const std::filesystem::path parent = std::filesystem::path(iniPath).parent_path();
		if(!parent.empty()) {
			std::filesystem::create_directories(parent);
		}

		// ImGui の現在設定を ini として保存する
		ImGui::SaveIniSettingsToDisk(iniPath.c_str());
	}

	////////////////////////////////////////////////////////////////////////////////////////////
	//		専用レイアウト読み込み
	////////////////////////////////////////////////////////////////////////////////////////////
	void ImGuiLayoutSwitcher::LoadExclusiveFromDisk(const std::string& iniPath) {
		if(iniPath.empty()) return;

		// 既存の Window / Docking 設定を消して、前レイアウトとのマージを防ぐ
		ImGui::ClearIniSettings();

		// 指定 ini だけを読み込み、専用レイアウトとして復元する
		ImGui::LoadIniSettingsFromDisk(iniPath.c_str());
	}

} // namespace CalyxEngine
