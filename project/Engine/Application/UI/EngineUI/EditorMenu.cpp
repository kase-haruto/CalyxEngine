#include "EditorMenu.h"

#include <externals/imgui/imgui.h>
namespace CalyxEditor {
	void EditorMenu::Add(MenuCategory category, const MenuItem& item) {
		items_[category].push_back(item);
	}

	const std::vector<MenuItem>& EditorMenu::Get(MenuCategory category) const {
		static const std::vector<MenuItem> empty;
		auto							   it = items_.find(category);
		return it != items_.end() ? it->second : empty;
	}

	void EditorMenu::Clear() {
		items_.clear();
	}

	void EditorMenu::RenderCategory(const char* label, MenuCategory category) {
		if(ImGui::BeginMenu(label)) {
			for(const auto& item : Get(category)) {
				if(ImGui::MenuItem(item.label.c_str(), item.shortcut.c_str(), false, item.enabled)) {
					item.action();
				}
			}
			ImGui::EndMenu();
		}
	}

	void EditorMenu::Render() {
		// ProcessShortcuts(ImGui::GetIO());

		if(ImGui::BeginMainMenuBar()) {
			RenderCategory("File(F)", MenuCategory::File);
			RenderCategory("View(V)", MenuCategory::View);
			RenderCategory("Edit(E)", MenuCategory::Edit);
			RenderCategory("Tools(T)", MenuCategory::Tools);
			ImGui::EndMainMenuBar();
		}
	}
} // namespace CalyxEditor