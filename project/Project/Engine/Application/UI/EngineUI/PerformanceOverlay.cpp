#include "PerformanceOverlay.h"

PerformanceOverlay::PerformanceOverlay(){
	align_ = OverlayAlign::TopRight;
	overlayOffset_ = ImVec2(-200.0f, 10.0f);
}

void PerformanceOverlay::RenderOverlay(const ImVec2& basePos){
	// FPS/FrameTime 測定
	static float fps = 0.0f;
	static float frameTime = 0.0f;
	static float smoothing = 0.05f;

	float currentFPS = ImGui::GetIO().Framerate;
	float currentFrameTime = 1000.0f / currentFPS;

	fps = fps * (1.0f - smoothing) + currentFPS * smoothing;
	frameTime = frameTime * (1.0f - smoothing) + currentFrameTime * smoothing;

	ImGui::SetCursorScreenPos(basePos);
	ImGui::Text("FPS: %.1f", fps);
	float space = 20.0f;
	ImVec2 btnPos = ImVec2(basePos.x, basePos.y + space);
	ImGui::SetCursorScreenPos(btnPos);
	ImGui::Text("Frame Time: %.1f ms", frameTime);
}

void PerformanceOverlay::RenderToolbar(){}
