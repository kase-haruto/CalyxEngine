#include "SpriteRenderer.h"
#include <Engine/Renderer/Sprite/Sprite.h>
#include <Engine/Graphics/Pipeline/Presets/PipelinePresets.h>
#include <Engine/Graphics/Pipeline/Service/PipelineService.h>

void SpriteRenderer::Register(Sprite* sprite){
	if (sprite){
		sprites_.push_back(sprite);
	}
}

void SpriteRenderer::Draw(ID3D12GraphicsCommandList* cmdList,
						  PipelineService* psoService,
						  RenderTargetType renderTarget){
	if (sprites_.empty()) return;

	sprites_.erase(
		std::remove_if(sprites_.begin(), sprites_.end(),
		[renderTarget] (Sprite* s){ return s->GetTargetRt() != renderTarget; }),
		sprites_.end());

	if (sprites_.empty()){ Clear(); return; }

	auto desc = PipelinePresets::MakeObject2D();
	psoService->SetCommand(desc, cmdList);

	for (Sprite* sprite : sprites_) sprite->Draw(cmdList);

	Clear();
}


void SpriteRenderer::Clear(){
	sprites_.clear();
}
