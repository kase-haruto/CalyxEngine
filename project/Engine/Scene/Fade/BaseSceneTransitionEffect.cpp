#include "BaseSceneTransitionEffect.h"

namespace CalyxEngine {

	BaseSceneTransitionEffect::BaseSceneTransitionEffect(float duration)
		: duration_(duration < 0.0f ? 0.0f : duration) {}

	void BaseSceneTransitionEffect::StartFadeOut() {
		// 再利用時の進行状態を初期化し、派生Effectへ開始時と0%状態を順に通知する。
		elapsed_ = 0.0f;
		progress_ = 0.0f;
		fadeOutFinished_ = false;
		fadeInFinished_ = false;
		OnStartFadeOut();
		OnFadeOut(progress_);
	}

	void BaseSceneTransitionEffect::FadeOutUpdate(float dt) {
		if(fadeOutFinished_) return;
		// 負のdeltaTimeを無視し、durationが0の場合は即時完了として扱う。
		elapsed_ += dt < 0.0f ? 0.0f : dt;
		progress_ = duration_ <= 0.0f ? 1.0f : elapsed_ / duration_;
		if(progress_ > 1.0f) progress_ = 1.0f;
		OnFadeOut(progress_);
		// 終了Callbackは進行度が初めて100%へ到達したときだけ呼び出す。
		if(progress_ >= 1.0f) {
			fadeOutFinished_ = true;
			OnEndFadeOut();
		}
	}

	void BaseSceneTransitionEffect::StartFadeIn() {
		// FadeOut完了状態は維持しつつ、FadeIn側の時間と完了Flagを初期化する。
		elapsed_ = 0.0f;
		progress_ = 0.0f;
		fadeInFinished_ = false;
		OnStartFadeIn();
		OnFadeIn(progress_);
	}

	void BaseSceneTransitionEffect::FadeInUpdate(float dt) {
		if(fadeInFinished_) return;
		// FadeOutと同一の正規化規則を使い、派生Effectの補間範囲を0～1へ保証する。
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
