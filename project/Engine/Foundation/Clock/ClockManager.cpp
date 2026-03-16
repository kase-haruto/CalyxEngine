#include "ClockManager.h"
#include <algorithm>
#include <chrono>

ClockManager::ClockManager() {
	firstFrameTime_ = std::chrono::high_resolution_clock::now();
	lastFrameTime_	= firstFrameTime_;
}

void ClockManager::Update() {
	auto now = std::chrono::high_resolution_clock::now();

	rawDeltaTime_ =
		std::chrono::duration<float>(now - lastFrameTime_).count();

	// 実 dt
	float dt = rawDeltaTime_;

	dt = std::clamp(dt, 0.0f, 0.017f);

	// グローバル dt 更新
	globalDeltaTime_ = dt;

	// スローモーション更新
	if(isSlowMotionActive_) {
		slowMotionElapsed_ += globalDeltaTime_;
		currentTimeScale_ = slowMotionScale_;
		if(slowMotionElapsed_ >= slowMotionDuration_) {
			isSlowMotionActive_ = false;
			currentTimeScale_	= 1.0f;
		}
	}

	playerDeltaTime_ = dt * currentTimeScale_;

	if(rawDeltaTime_ > 0.0f) {
		currentFPS_ = 1.0f / rawDeltaTime_;
	}

	lastFrameTime_ = now;
}
float ClockManager::GetRawDeltaTime() const { return rawDeltaTime_; }
void  ClockManager::StartHitStop(float duration) {
	 isHitStopActive_ = true;
	 hitStopDuration_ = duration;
	 hitStopElapsed_  = 0.0f;
}

void ClockManager::StartSlowMotion(float scale, float duration) {
	isSlowMotionActive_ = true;
	slowMotionScale_	= scale;
	slowMotionDuration_ = duration;
	slowMotionElapsed_	= 0.0f;
}