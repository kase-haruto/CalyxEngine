#include "ManipulatorSettingsUI.h"
#include "Engine/Assets/Manager/AssetManager.h"
#include <externals/imgui/imgui.h>

namespace CalyxEngine::ManipulatorSettingsUI {

	void Render(ManipulatorSettings& settings) {
		ImGui::Checkbox("Enable snap", &settings.useSnap);
		if(settings.useSnap) {
			ImGui::SetNextItemWidth(140.0f);
			ImGui::InputFloat3("Move step", settings.snapTranslate);
			ImGui::SetNextItemWidth(140.0f);
			ImGui::InputFloat("Angle step", &settings.snapRotate);
			ImGui::SetNextItemWidth(140.0f);
			ImGui::InputFloat3("Scale step", settings.snapScale);
		}
	}

} // namespace CalyxEngine::ManipulatorSettingsUI