#pragma once

#include <Engine/Graphics/RenderTarget/Detail/RenderTargetDetail.h>

#include <vector>
#include <d3d12.h>

class Sprite;

class SpriteRenderer {
public:
	void Register(Sprite* sprite);
	void Draw(ID3D12GraphicsCommandList* cmdList,
			  class PipelineService* psoService,
			  RenderTargetType);
	void Clear();

private:
	std::vector<Sprite*> sprites_;
};