#pragma once

#include <Engine/Foundation/Export/CalyxAPI.h>

#include <string>

class PipelineService;
struct ID3D12GraphicsCommandList;

namespace CalyxEngine {

	// Base class for effects that cover the old scene, then reveal the new scene.
	// Game projects can derive from this class and supply their own Update/Draw hooks.
	/**
	 * @brief BaseSceneTransitionEffectの機能を提供するクラスです。
	 */
	class CALYX_API BaseSceneTransitionEffect {
	public:
		explicit BaseSceneTransitionEffect(float duration = 0.5f);
		virtual ~BaseSceneTransitionEffect() = default;

		void StartFadeOut();
		void FadeOutUpdate(float dt);
		void StartFadeIn();
		void FadeInUpdate(float dt);

		bool IsFadeOutFinished() const { return fadeOutFinished_; }
		bool IsFadeInFinished() const { return fadeInFinished_; }
		float GetProgress() const { return progress_; }

		virtual void Draw(ID3D12GraphicsCommandList* cmd, PipelineService* pso) = 0;

		void SetTime(float duration) { duration_ = duration < 0.0f ? 0.0f : duration; }
		float GetTime() const { return duration_; }

	protected:
		virtual void OnStartFadeOut() {}
		virtual void OnFadeOut(float normalizedTime) { (void)normalizedTime; }
		virtual void OnEndFadeOut() {}
		virtual void OnStartFadeIn() {}
		virtual void OnFadeIn(float normalizedTime) { (void)normalizedTime; }
		virtual void OnEndFadeIn() {}

	private:
		float duration_ = 0.5f;
		float elapsed_ = 0.0f;
		float progress_ = 0.0f;
		bool fadeOutFinished_ = false;
		bool fadeInFinished_ = false;
	};

} // namespace CalyxEngine
