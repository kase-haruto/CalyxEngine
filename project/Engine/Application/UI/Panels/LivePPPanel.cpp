#include "LivePPPanel.h"
#include "../../../Foundation/HotReload/LivePP/LivePPService.h"
#include <externals/imgui/imgui.h>

namespace CalyxEngine {

	LivePPPanel::LivePPPanel() : IEngineUI("Hot Reload (Live++)") {
	}

	void LivePPPanel::Render() {
		bool isopen = IsShow();
		if(!ImGui::Begin(panelName_.c_str(), &isopen)) {
			ImGui::End();
			return;
		}

		auto* service = CalyxEngine::LivePPService::GetInstance();
		if(!service) {
			ImGui::TextColored(ImVec4(1, 0, 0, 1), "LivePPService not found!");
			ImGui::End();
			return;
		}

		auto status = service->GetStatus();

		// Info

		// Progress Bar (Mockup - Live++ doesn't give fine-grained progress via API easily)
		if(status == CalyxEngine::LivePPStatus::Compiling || status == CalyxEngine::LivePPStatus::Patching) {
			float time	   = (float)ImGui::GetTime();
			float progress = fmodf(time, 2.0f) / 2.0f;
			ImGui::ProgressBar(progress, ImVec2(-1, 0), "Processing...");
		}

		// Error Output
		if(status == CalyxEngine::LivePPStatus::Error) {
			ImGui::TextUnformatted("Compiler Output:");
			ImGui::BeginChild("ErrorLog", ImVec2(0, 0), true, ImGuiWindowFlags_HorizontalScrollbar);
			ImGui::TextUnformatted(service->GetLastCompilerOutput().c_str());
			ImGui::EndChild();
		}

		ImGui::End();
		if(!isopen) {
			SetShow(false);
		}
	}

} // namespace CalyxEngine
