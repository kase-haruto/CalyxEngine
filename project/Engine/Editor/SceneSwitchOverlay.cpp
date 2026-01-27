#include "SceneSwitchOverlay.h"
#include <string>
#include <externals/imgui/imgui.h>
#include <Engine/System/Command/EditorCommand/GuiCommand/ImGuiHelper/GuiCmd.h>
#include <vector>


namespace CalyxEditor {

	void SceneSwitchOverlay::RenderToolbar() {
		if (!sceneManager_) return;

		ImGui::Begin("Scene Switcher", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
		
		const auto& ids = sceneManager_->GetRegisteredSceneIds();

		// コンボボックス用のデータ作成
		std::vector<std::string> names;
		std::vector<const char*> items;
		names.reserve(ids.size());
		items.reserve(ids.size());

		for (auto id : ids) {
			names.push_back(sceneManager_->GetSceneName(id));
			items.push_back(names.back().c_str());
		}

		int currentIdx = static_cast<int>(sceneManager_->GetCurrentIndex());

		// GuiCmdを使用してコンボボックス表示
		if (GuiCmd::Combo("Scene", currentIdx, items.data(), static_cast<int>(items.size()))) {
			if (currentIdx >= 0 && currentIdx < static_cast<int>(ids.size())) {
				sceneManager_->GetTransitionRequestor().RequestSceneChange(ids[currentIdx]);
			}
		}

		ImGui::End();
	}

} // namespace CalyxEditor