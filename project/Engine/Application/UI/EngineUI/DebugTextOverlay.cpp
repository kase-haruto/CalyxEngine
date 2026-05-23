#include "DebugTextOverlay.h"
#include "DebugTextManager.h"

#include <algorithm>
#include <cstdio>

namespace CalyxEngine {
//////////////////////////////////////////////////////////////////////////
///		コンストラクタ/デストラクタ
//////////////////////////////////////////////////////////////////////////
DebugTextOverlay::DebugTextOverlay() {
	BaseOnViewportTool::SetOverlayAlign(OverlayAlign::TopLeft);
	BaseOnViewportTool::SetOverlayOffset(ImVec2(0.0f, 0.0f));
}
DebugTextOverlay::~DebugTextOverlay() = default;

//////////////////////////////////////////////////////////////////////////
///		表示
//////////////////////////////////////////////////////////////////////////
void DebugTextOverlay::RenderOverlay([[maybe_unused]] const ImVec2& basePos) {
}

void DebugTextOverlay::RenderGlobalPopups() {
	const auto& popups = DebugTextManager::GetPopupTexts();
	if(popups.empty()) return;

	const ImGuiViewport* viewport = ImGui::GetMainViewport();
	if(!viewport) return;

	const ImVec2 workPos	 = viewport->WorkPos;
	const ImVec2 workSize = viewport->WorkSize;
	const float  margin	 = 18.0f;
	const float  width	 = 360.0f;
	const float  padding = 14.0f;
	const float  iconArea = 38.0f;
	const float  spacing = 10.0f;

	float stackOffset = 0.0f;

	for(size_t index = 0; index < popups.size(); ++index) {
		const auto& popup = popups[index];
		const float lifetime = std::max(popup.lifetime, 0.001f);
		const float t		  = std::clamp(popup.age / lifetime, 0.0f, 1.0f);
		const float inT	  = std::clamp(popup.age / 0.25f, 0.0f, 1.0f);
		const float outT	  = std::clamp((t - 0.68f) / 0.32f, 0.0f, 1.0f);
		const float inEase  = 1.0f - (1.0f - inT) * (1.0f - inT);
		const float outEase = outT * outT * (3.0f - 2.0f * outT);
		const float alpha	  = 1.0f - outEase;
		const float slideX  = (1.0f - inEase) * (width + margin);
		const float riseY	  = popup.rise * outEase;

		const float textWidth = width - padding * 2.0f - iconArea;
		const ImVec2 textSize = ImGui::CalcTextSize(popup.text.c_str(), nullptr, false, textWidth);
		const float  height   = std::max(64.0f, textSize.y + padding * 2.0f);

		const ImVec2 pos(
			workPos.x + workSize.x - width - margin - popup.position.x + slideX,
			workPos.y + workSize.y - height - margin - popup.position.y - stackOffset - riseY);

		ImGui::SetNextWindowViewport(viewport->ID);
		ImGui::SetNextWindowPos(pos, ImGuiCond_Always);
		ImGui::SetNextWindowSize(ImVec2(width, height), ImGuiCond_Always);
		ImGui::SetNextWindowBgAlpha(0.0f);

		const ImGuiWindowFlags flags =
			ImGuiWindowFlags_NoDecoration |
			ImGuiWindowFlags_NoSavedSettings |
			ImGuiWindowFlags_NoFocusOnAppearing |
			ImGuiWindowFlags_NoNav |
			ImGuiWindowFlags_NoMove |
			ImGuiWindowFlags_NoInputs;

		char windowName[64];
		snprintf(windowName, sizeof(windowName), "DebugToastPopup_%zu", index);

		if(ImGui::Begin(windowName, nullptr, flags)) {
			ImDrawList* drawList = ImGui::GetWindowDrawList();
			const ImVec2 rectMin = ImGui::GetWindowPos();
			const ImVec2 rectMax(rectMin.x + width, rectMin.y + height);
			const ImU32  bgColor = ImGui::ColorConvertFloat4ToU32(ImVec4(0.10f, 0.12f, 0.14f, 0.94f * alpha));
			const ImU32  borderColor = ImGui::ColorConvertFloat4ToU32(ImVec4(0.0f, 0.0f, 0.0f, 0.55f * alpha));
			const ImVec4 accent = ImVec4(popup.color.x, popup.color.y, popup.color.z, popup.color.w * alpha);
			const ImU32  accentColor = ImGui::ColorConvertFloat4ToU32(accent);
			const ImU32  textColor = ImGui::ColorConvertFloat4ToU32(ImVec4(0.92f, 0.94f, 0.96f, alpha));

			drawList->AddRectFilled(rectMin, rectMax, bgColor, 6.0f);
			drawList->AddRect(rectMin, rectMax, borderColor, 6.0f);

			const ImVec2 iconCenter(rectMin.x + padding + 12.0f, rectMin.y + padding + 12.0f);
			drawList->AddTriangle(
				ImVec2(iconCenter.x, iconCenter.y - 14.0f),
				ImVec2(iconCenter.x - 14.0f, iconCenter.y + 12.0f),
				ImVec2(iconCenter.x + 14.0f, iconCenter.y + 12.0f),
				accentColor,
				2.0f);
			drawList->AddText(ImVec2(iconCenter.x - 3.0f, iconCenter.y - 5.0f), accentColor, "!");

			ImGui::SetCursorScreenPos(ImVec2(rectMin.x + padding + iconArea, rectMin.y + padding));
			ImGui::PushTextWrapPos(rectMin.x + width - padding);
			ImGui::PushStyleColor(ImGuiCol_Text, textColor);
			ImGui::TextUnformatted(popup.text.c_str());
			ImGui::PopStyleColor();
			ImGui::PopTextWrapPos();
		}
		ImGui::End();

		stackOffset += height + spacing;
	}
}

void DebugTextOverlay::RenderFatalAssertWindow() {
	if(!DebugTextManager::HasFatalAssert()) return;

	const auto& fatal = DebugTextManager::GetFatalAssert();
	const ImGuiViewport* viewport = ImGui::GetMainViewport();
	if(!viewport) return;

	ImGui::OpenPopup("Fatal Assert");

	const ImVec2 center(
		viewport->WorkPos.x + viewport->WorkSize.x * 0.5f,
		viewport->WorkPos.y + viewport->WorkSize.y * 0.5f);
	ImGui::SetNextWindowViewport(viewport->ID);
	ImGui::SetNextWindowPos(center, ImGuiCond_Always, ImVec2(0.5f, 0.5f));
	ImGui::SetNextWindowSize(ImVec2(520.0f, 0.0f), ImGuiCond_Always);

	const ImGuiWindowFlags flags =
		ImGuiWindowFlags_NoSavedSettings |
		ImGuiWindowFlags_NoResize |
		ImGuiWindowFlags_NoCollapse;

	if(ImGui::BeginPopupModal("Fatal Assert", nullptr, flags)) {
		ImDrawList* drawList = ImGui::GetWindowDrawList();
		const ImVec2 windowPos = ImGui::GetWindowPos();
		const ImVec2 iconCenter(windowPos.x + 34.0f, windowPos.y + 50.0f);
		const ImU32 accentColor = ImGui::ColorConvertFloat4ToU32(fatal.color);

		drawList->AddTriangle(
			ImVec2(iconCenter.x, iconCenter.y - 17.0f),
			ImVec2(iconCenter.x - 17.0f, iconCenter.y + 15.0f),
			ImVec2(iconCenter.x + 17.0f, iconCenter.y + 15.0f),
			accentColor,
			2.0f);
		drawList->AddText(ImVec2(iconCenter.x - 3.0f, iconCenter.y - 7.0f), accentColor, "!");

		ImGui::Indent(54.0f);
		ImGui::TextColored(fatal.color, "Fatal assertion failed");
		ImGui::Spacing();
		ImGui::TextWrapped("Expression: %s", fatal.expression.c_str());
		if(!fatal.message.empty()) {
			ImGui::TextWrapped("Message: %s", fatal.message.c_str());
		}
		ImGui::TextWrapped("Location: %s:%d", fatal.file.c_str(), fatal.line);
		ImGui::Spacing();
		ImGui::TextWrapped("Press OK to break execution.");
		ImGui::Unindent(54.0f);

		ImGui::Separator();
		const float buttonWidth = 96.0f;
		ImGui::SetCursorPosX(ImGui::GetWindowWidth() - buttonWidth - 16.0f);
		if(ImGui::Button("OK", ImVec2(buttonWidth, 0.0f))) {
			DebugTextManager::RequestBreak();
			ImGui::CloseCurrentPopup();
		}
		ImGui::EndPopup();
	}

	if(DebugTextManager::ConsumeBreakRequest()) {
		__debugbreak();
	}
}
} // namespace CalyxEngine
