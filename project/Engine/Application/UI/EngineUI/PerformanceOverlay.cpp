#include "PerformanceOverlay.h"

#include <Engine/Foundation/Clock/ClockManager.h>

namespace CalyxEditor {

	PerformanceOverlay::PerformanceOverlay() {
		align_		   = OverlayAlign::TopRight;
		overlayOffset_ = ImVec2(-200.0f, 10.0f);

		showOverlay_ = true;
		color_		 = ImVec4(1.0f, 1.0f, 1.0f, 1.0f); // 白
	}

	void PerformanceOverlay::RenderOverlay(const ImVec2& basePos) {
		if(!showOverlay_) {
			return; // 早期リターン
		}
		auto*				   clock		   = ClockManager::GetInstance();
		static float		   smoothFPS	   = 0.0f;
		static float		   smoothFrameTime = 0.0f;
		static constexpr float smoothing	   = 0.05f;

		float fps		= clock->GetCurrentFPS();
		float frameTime = 1000.0f / fps;

		// スムージング
		smoothFPS		= smoothFPS * (1.0f - smoothing) + fps * smoothing;
		smoothFrameTime = smoothFrameTime * (1.0f - smoothing) + frameTime * smoothing;

		ImGui::SetCursorScreenPos(basePos);
		ImGui::Text("FPS: %.1f", smoothFPS);

		ImVec2 next = basePos;
		next.y += 20.0f;
		ImGui::SetCursorScreenPos(next);
		ImGui::Text("Frame Time: %.1f ms", smoothFrameTime);

		next.y += 20.0f;
		ImGui::SetCursorScreenPos(next);
		ImGui::Text("DeltaTime: %.3f", clock->GetDeltaTime());
	}

	void PerformanceOverlay::RenderToolbar() {
		ImGui::Begin("PerformanceView", nullptr,
					 ImGuiWindowFlags_NoTitleBar |
						 ImGuiWindowFlags_AlwaysAutoResize |
						 ImGuiWindowFlags_NoResize);

		// オーバーレイのON/OFF
		ImGui::Checkbox("Show Overlay", &showOverlay_);
		ImGui::SameLine();
		ImGui::Checkbox("isAdjustment", &isAdjustment_);

		ImGui::End();

		if(isAdjustment_) {
			// パラメータ調整ウィンドウは別に出す
			ImGui::Begin("PerformanceOverlayParms");
			// 色選択
			ImGui::ColorEdit4("Overlay Color", (float*)&color_, ImGuiColorEditFlags_NoInputs);

			// 位置調整
			ImGui::Text("Position Offset");
			ImGui::DragFloat2("Offset", (float*)&overlayOffset_, 1.0f);
			ImGui::End();
		}
	}

} // namespace CalyxEditor
