#pragma once
#include <functional>
#include <string>
#include <unordered_map>
#include <vector>

enum class MenuCategory {
	File,
	Edit,
	View,
	Tools
};

struct MenuItem {
	std::string label;
	std::string shortcut;
	std::function<void()> action;
	bool enabled = true;
};

/// <summary>
/// エディタのメニュー
/// </summary>
class EditorMenu {
public:
	/// <summary>
	/// 追加
	/// </summary>
	/// <param name="category"></param>
	/// <param name="item"></param>
	void Add(MenuCategory category, const MenuItem& item);

	/// <summary>
	/// 取得
	/// </summary>
	/// <param name="category"></param>
	/// <returns>メニューアイテム</returns>
	const std::vector<MenuItem>& Get(MenuCategory category) const;

	/// <summary>
	/// クリア
	/// </summary>
	void Clear();

	/// <summary>
	/// 描画
	/// </summary>
	void Render();

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