#include "PerformanceOverlay.h"

#include <Engine/Foundation/Clock/ClockManager.h>

namespace CalyxEditor {

    PerformanceOverlay::PerformanceOverlay() {
        align_         = OverlayAlign::TopRight;
        overlayOffset_ = ImVec2(-200.0f, 10.0f);

        showOverlay_ = true;
        color_       = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
    }

    void PerformanceOverlay::RenderOverlay(const ImVec2& basePos) {
        if (!showOverlay_) return;

        auto* clock = ClockManager::GetInstance();

        //-------------------------------------------------------
        // Clamp の影響を受けない
        //-------------------------------------------------------
        float fps = clock->GetCurrentFPS();
        float rawDt = clock->GetRawDeltaTime();
        float frameTime = rawDt * 1000.0f;

        //-------------------------------------------------------
        // スムージング（FPS, ms）
        //-------------------------------------------------------
        static float smoothFPS = 0.0f;
        static float smoothFrameTime = 0.0f;
        constexpr float smoothing = 0.15f;

        if (fps > 0.0f) {
            smoothFPS       = smoothFPS * (1.0f - smoothing) + fps * smoothing;
            smoothFrameTime = smoothFrameTime * (1.0f - smoothing) + frameTime * smoothing;
        }

        //-------------------------------------------------------
        // 表示
        //-------------------------------------------------------
        ImGui::SetCursorScreenPos(basePos);
        ImGui::Text("FPS: %.1f", smoothFPS);

        ImVec2 next = basePos;
        next.y += 20.0f;
        ImGui::SetCursorScreenPos(next);
        ImGui::Text("Frame Time: %.2f ms", smoothFrameTime);

        next.y += 20.0f;
        ImGui::SetCursorScreenPos(next);
        ImGui::Text("DeltaTime(raw): %.4f", rawDt);
    }

    void PerformanceOverlay::RenderToolbar() {
        ImGui::Begin("PerformanceView", nullptr,
            ImGuiWindowFlags_NoTitleBar |
            ImGuiWindowFlags_AlwaysAutoResize |
            ImGuiWindowFlags_NoResize);

        ImGui::Checkbox("Show Overlay", &showOverlay_);
        ImGui::SameLine();
        ImGui::Checkbox("isAdjustment", &isAdjustment_);

        ImGui::End();

        if (isAdjustment_) {
            ImGui::Begin("PerformanceOverlayParms");
            ImGui::ColorEdit4("Overlay Color", (float*)&color_, ImGuiColorEditFlags_NoInputs);
            ImGui::Text("Position Offset");
            ImGui::DragFloat2("Offset", (float*)&overlayOffset_, 1.0f);
            ImGui::End();
        }
    }

} // namespace CalyxEditor