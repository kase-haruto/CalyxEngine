#include "FadeBlackOutEffect.h"

#include <Engine/Graphics/Pipeline/Presets/PipelinePresets.h>
#include <Engine/Graphics/Pipeline/Service/PipelineService.h>
#include <Engine/Renderer/Sprite/Sprite.h>

namespace CalyxEngine {

	FadeBlackOutEffect::FadeBlackOutEffect(float duration)
		: BaseSceneTransitionEffect(duration) {}

	FadeBlackOutEffect::~FadeBlackOutEffect() = default;

	void FadeBlackOutEffect::EnsureSprite() {
		if(sprite_) return;
		sprite_ = std::make_unique<Sprite>("white1x1.png");
		sprite_->Initialize({0.0f, 0.0f}, {1280.0f, 720.0f});
		sprite_->SetColor({0.0f, 0.0f, 0.0f, 0.0f});
	}

	void FadeBlackOutEffect::OnStartFadeOut() { EnsureSprite(); }

	void FadeBlackOutEffect::OnFadeOut(float normalizedTime) {
		EnsureSprite();
		sprite_->SetAlpha(normalizedTime);
		sprite_->Update();
	}

	void FadeBlackOutEffect::OnFadeIn(float normalizedTime) {
		EnsureSprite();
		sprite_->SetAlpha(1.0f - normalizedTime);
		sprite_->Update();
	}

	void FadeBlackOutEffect::Draw(ID3D12GraphicsCommandList* cmd, PipelineService* pso) {
		if(!sprite_ || !cmd || !pso) return;
		pso->SetCommand(PipelinePresets::MakeObject2D(), cmd);
		sprite_->Draw(cmd);
	}

} // namespace CalyxEngine
