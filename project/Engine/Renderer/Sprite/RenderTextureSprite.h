#pragma once

#include <Engine/Foundation/Export/CalyxAPI.h>
#include <Engine/Graphics/Descriptor/DescriptorAllocator.h>
#include <Engine/Graphics/RenderTarget/OffscreenRT/OffscreenRenderTarget.h>
#include <Engine/Renderer/Sprite/Sprite.h>

#include <cstdint>
#include <memory>

/**
 * Owns an offscreen color/depth target and exposes its color SRV as a Sprite.
 * Record 3D commands between Begin3D() and End3D(), then submit GetSprite()
 * to SpriteRenderer after End3D().
 */
class CALYX_API RenderTextureSprite {
public:
	RenderTextureSprite() = default;
	~RenderTextureSprite();

	RenderTextureSprite(const RenderTextureSprite&) = delete;
	RenderTextureSprite& operator=(const RenderTextureSprite&) = delete;

	bool Initialize(uint32_t width, uint32_t height,
		DXGI_FORMAT format = DXGI_FORMAT_R8G8B8A8_UNORM);
	void Resize(uint32_t width, uint32_t height);

	void Begin3D(ID3D12GraphicsCommandList* commandList);
	void End3D(ID3D12GraphicsCommandList* commandList);

	void SetClearColor(float r, float g, float b, float a);
	Sprite* GetSprite() const { return sprite_.get(); }
	OffscreenRenderTarget* GetRenderTarget() { return renderTarget_.get(); }
	const OffscreenRenderTarget* GetRenderTarget() const { return renderTarget_.get(); }
	bool IsInitialized() const { return renderTarget_ != nullptr; }

private:
	void ReleaseDescriptors();

	std::unique_ptr<OffscreenRenderTarget> renderTarget_;
	std::unique_ptr<Sprite> sprite_;
	DescriptorHandle rtvHandle_{};
	DescriptorHandle dsvHandle_{};
	bool recording3D_ = false;
};
