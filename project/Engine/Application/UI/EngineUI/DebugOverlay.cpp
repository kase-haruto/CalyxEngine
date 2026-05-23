#include "DebugOverlay.h"
#include "DebugTextManager.h"
#include <externals/imgui/imgui.h>

namespace CalyxEngine {

	DebugOverlay::DebugOverlay() {
		SetOverlayAlign(OverlayAlign::CenterTop);
		SetOverlayOffset(ImVec2(0, 50));
	}

	void DebugOverlay::RenderOverlay(const ImVec2& basePos) {
		const auto& messages = DebugTextManager::GetMessages();
		if(messages.empty()) return;

		ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize |
								 ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing |
								 ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoInputs;

		ImGui::SetNextWindowPos(basePos, ImGuiCond_Always, ImVec2(0.5f, 0.0f));
		ImGui::SetNextWindowBgAlpha(0.7f);

		if(ImGui::Begin("DebugOverlayMessage", nullptr, flags)) {
			ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[0]); // デフォルトフォント

			for(const auto& msg : messages) {
				if(!msg.title.empty()) {
					ImGui::TextColored(msg.color, "%s", msg.title.c_str());
				}
				if(!msg.body.empty()) {
					ImGui::Text("%s", msg.body.c_str());
				}
				ImGui::Spacing();
			}

			ImGui::PopFont();
		}
		ImGui::End();
	}

} // namespace CalyxEngine
