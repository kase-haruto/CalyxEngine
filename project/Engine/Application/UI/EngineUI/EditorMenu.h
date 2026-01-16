#pragma once
/* ========================================================================
/*		include space
/* ===================================================================== */
#include <functional>
#include <string>
#include <unordered_map>
#include <vector>

namespace CalyxEditor {
	/* ========================================================================
/*		メニューカテゴリ
/* ===================================================================== */
	enum class MenuCategory {
		File,
		Edit,
		View,
		Tools
	};

	/* ========================================================================
	/*		メニューアイテム
	/* ===================================================================== */
	struct MenuItem {
		std::string			  label;
		std::string			  shortcut;
		std::function<void()> action;
		bool				  enabled = true;
	};

	/*-----------------------------------------------------------------------------------------
	 * EditorMenu
	 * - エディタメニュークラス
	 * - ファイル・編集・ビュー・ツールなどのメニュー項目を管理・描画
	 *---------------------------------------------------------------------------------------*/
	class EditorMenu {
	public:
		/// <summary>
		/// 追加
		/// </summary>
		/// <param name="category"></param>
		/// <param name="item"></param>
		void Add(MenuCategory category, const MenuItem& item);

		/// <summary>
		/// クリア
		/// </summary>
		void Clear();

		/// <summary>
		/// 描画
		/// </summary>
		void Render();

		//--------- accessor -----------------------------------------------------
		const std::vector<MenuItem>& Get(MenuCategory category) const;

	private:
		/// <summary>
		/// カテゴリ描画
		/// </summary>
		/// <param name="label"></param>
		/// <param name="category"></param>
		void RenderCategory(const char* label, MenuCategory category);

	private:
		std::unordered_map<MenuCategory, std::vector<MenuItem>> items_;
	};

} // namespace CalyxEditor