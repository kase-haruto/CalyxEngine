#include "BaseSceneTransitionEffect.h"

namespace CalyxEngine {

	BaseSceneTransitionEffect::BaseSceneTransitionEffect(float duration)
		: duration_(duration < 0.0f ? 0.0f : duration) {}

	void BaseSceneTransitionEffect::StartFadeOut() {
		elapsed_ = 0.0f;
		progress_ = 0.0f;
		fadeOutFinished_ = false;
		fadeInFinished_ = false;
		OnStartFadeOut();
		OnFadeOut(progress_);
	}

	void BaseSceneTransitionEffect::FadeOutUpdate(float dt) {
		if(fadeOutFinished_) return;
		elapsed_ += dt < 0.0f ? 0.0f : dt;
		progress_ = duration_ <= 0.0f ? 1.0f : elapsed_ / duration_;
		if(progress_ > 1.0f) progress_ = 1.0f;
		OnFadeOut(progress_);
		if(progress_ >= 1.0f) {
			fadeOutFinished_ = true;
			OnEndFadeOut();
		}
	}

	void BaseSceneTransitionEffect::StartFadeIn() {
		elapsed_ = 0.0f;
		progress_ = 0.0f;
		fadeInFinished_ = false;
		OnStartFadeIn();
		OnFadeIn(progress_);
	}

	void BaseSceneTransitionEffect::FadeInUpdate(float dt) {
		if(fadeInFinished_) return;
		elapsed_ += dt < 0.0f ? 0.0f : dt;
		progress_ = duration_ <= 0.0f ? 1.0f : elapsed_ / duration_;
		if(progress_ > 1.0f) progress_ = 1.0f;
		OnFadeIn(progress_);
		if(progress_ >= 1.0f) {
			fadeInFinished_ = true;
			OnEndFadeIn();
		}
	}

} // namespace CalyxEngine
