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

class EditorMenu {
public:
	void Add(MenuCategory category, const MenuItem& item);
	const std::vector<MenuItem>& Get(MenuCategory category) const;
	void Clear();
	void Render();

private:
	void RenderCategory(const char* label, MenuCategory category);

private:
	std::unordered_map<MenuCategory, std::vector<MenuItem>> items_;
};