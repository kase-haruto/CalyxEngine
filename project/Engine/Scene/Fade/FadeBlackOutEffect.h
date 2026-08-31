#pragma once

#include "BaseSceneTransitionEffect.h"
#include <memory>

class Sprite;

namespace CalyxEngine {

	/**
	 * @brief FadeBlackOutEffectの機能を提供するクラスです。
	 */
	class CALYX_API FadeBlackOutEffect final : public BaseSceneTransitionEffect {
	public:
		explicit FadeBlackOutEffect(float duration = 0.5f);
		~FadeBlackOutEffect() override;

		void Draw(ID3D12GraphicsCommandList* cmd, PipelineService* pso) override;

	protected:
		void OnStartFadeOut() override;
		void OnFadeOut(float normalizedTime) override;
		void OnFadeIn(float normalizedTime) override;

	private:
		void EnsureSprite();
		std::unique_ptr<Sprite> sprite_;
	};

} // namespace CalyxEngine
