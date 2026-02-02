#pragma once
#include <string>
#include <vector>

namespace CalyxEditor {

	/// <summary>
	/// レイアウトエントリ
	/// - レイアウト名とファイルパスのペア
	/// </summary>
	struct LayoutEntry {
		std::string name;
		std::string path;
	};

	/*-----------------------------------------------------------------------------------------
	 * ImGuiLayoutSwitcher
	 * - ImGuiレイアウト切り替えクラス
	 * - 複数のレイアウトプリセットを管理し、.iniファイルの切り替えを可能にする
	 * - メニューからレイアウトの選択、保存、リロードが可能
	 *---------------------------------------------------------------------------------------*/
	class ImGuiLayoutSwitcher {
	public:
		/// <summary>
		/// コンストラクタ
		/// </summary>
		/// <param name="presets">プリセットレイアウトのリスト</param>
		/// <param name="defaultPath">デフォルトの.iniファイルパス</param>
		ImGuiLayoutSwitcher(std::vector<LayoutEntry> presets,
							std::string				 defaultPath = "imgui.ini");

		/// <summary>
		/// レイアウトメニューを描画
		/// - プリセット選択
		/// - リロード、保存、名前を付けて保存
		/// </summary>
		void DrawMenu();

		/// <summary>
		/// 指定されたレイアウトを適用
		/// </summary>
		/// <param name="iniPath">適用する.iniファイルのパス</param>
		void Apply(const std::string& iniPath);

		/// <summary>
		/// 現在のレイアウトパスを取得
		/// </summary>
		const std::string& GetCurrentPath() const { return currentIniPath_; }

	private:
		std::vector<LayoutEntry> presets_;		  ///< プリセットレイアウトのリスト
		std::string				 currentIniPath_; ///< 現在の.iniファイルパス (プリセットとしてロードされたパス)
		std::string				 autoSavePath_;	  ///< 自動保存用の.iniファイルパス
	};

} // namespace CalyxEditor
