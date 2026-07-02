#include "SceneSwitchOverlay.h"

#include <externals/imgui/imgui.h>
#include <string>

namespace CalyxEngine {
	void SceneSwitchOverlay::RenderToolbar() {
		if(!sceneManager_) return;

		ImGui::Begin("Scene", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
		const std::string currentPath = sceneManager_->GetCurrentScenePath().generic_string();
		ImGui::TextUnformatted(currentPath.empty() ? "No scene open" : currentPath.c_str());
		ImGui::End();
	}
}
