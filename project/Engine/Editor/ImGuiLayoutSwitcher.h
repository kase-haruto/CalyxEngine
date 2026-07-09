#pragma once

#include <string>
#include <string_view>
#include <vector>

namespace CalyxEngine {

	/// <summary>
	/// レイアウトプリセット情報
	/// </summary>
	struct LayoutEntry {
		std::string name; //< メニュー表示名
		std::string path; //< ini ファイルパス
	};

	/*-----------------------------------------------------------------------------------------
	 * ImGuiLayoutSwitcher
	 * - ImGui の Docking / Window レイアウトを ini ファイル単位で管理する
	 * - レイアウト適用時は現在レイアウトを保存し、既存設定をクリアしてから専用 ini を読み込む
	 * - ゲーム側エディタ拡張の layoutPath も同じ仕組みで専用レイアウトとして扱う
	 *---------------------------------------------------------------------------------------*/
	class ImGuiLayoutSwitcher {
	public:
		/**
		 * @brief コンストラクタ
		 * @param presets メニューに表示するレイアウトプリセット
		 * @param defaultPath 自動保存に使う初期 ini ファイルパス
		 */
		ImGuiLayoutSwitcher(std::vector<LayoutEntry> presets,
							std::string				 defaultPath = "imgui.ini");

		/**
		 * @brief レイアウトメニューを描画する
		 */
		void DrawMenu();

		/**
		 * @brief 指定レイアウトの適用を予約する
		 * @param iniPath 適用する ini ファイルパス
		 */
		void Apply(const std::string& iniPath);

		/**
		 * @brief 予約されたレイアウト適用を UI 描画前の安全なタイミングで実行する
		 */
		void ApplyPending();

		/**
		 * @brief 現在の ImGui レイアウトを指定 ini ファイルへ保存する
		 * @param iniPath 保存先 ini ファイルパス
		 */
		void Save(const std::string& iniPath);

		/**
		 * @brief 現在選択中の ini ファイルへ保存する
		 */
		void SaveCurrent();

		/**
		 * @brief 現在選択中の ini ファイルを専用レイアウトとして再読み込みする
		 */
		void ReloadCurrent();

		/**
		 * @brief 現在選択中の ini ファイルパスを取得する
		 */
		const std::string& GetCurrentPath() const { return currentIniPath_; }

		/**
		 * @brief 登録済みレイアウトプリセットを取得する
		 */
		const std::vector<LayoutEntry>& GetPresets() const { return presets_; }

	private:
		/**
		 * @brief ini ファイルパスを実ファイルパスへ解決する
		 * @param path 解決対象パス
		 */
		std::string ResolveLayoutPath(std::string_view path) const;

		/**
		 * @brief ImGui の現在レイアウトを指定 ini ファイルへ保存する
		 * @param iniPath 保存先 ini ファイルパス
		 */
		void SaveToDisk(const std::string& iniPath) const;

		/**
		 * @brief 指定 ini ファイルを専用レイアウトとして読み込む
		 * @param iniPath 読み込み元 ini ファイルパス
		 */
		void LoadExclusiveFromDisk(const std::string& iniPath);

		std::vector<LayoutEntry> presets_;			  //< レイアウトプリセット一覧
		std::string				 currentIniPath_;	  //< 現在選択中の ini ファイルパス
		std::string				 autoSavePath_;		  //< ImGui の自動保存先パス
		std::string				 pendingIniPath_;	  //< 次フレームで適用する ini ファイルパス
		std::string				 saveBeforeApplyPath_; //< 切り替え前に保存する ini ファイルパス
		bool					 hasPendingApply_ = false; //< レイアウト適用予約があるか
	};

} // namespace CalyxEngine
