#include "RenderTextureSprite.h"

#include <Engine/Graphics/Context/GraphicsGroup.h>

RenderTextureSprite::~RenderTextureSprite() {
	// GPU ownership/lifetime follows the same rule as the other renderer resources:
	// destroy this object only after commands using it have completed.
	sprite_.reset();
	renderTarget_.reset();
	ReleaseDescriptors();
}

bool RenderTextureSprite::Initialize(uint32_t width, uint32_t height, DXGI_FORMAT format) {
	if(width == 0 || height == 0 || renderTarget_) return false;

	rtvHandle_ = DescriptorAllocator::Allocate(DescriptorUsage::Rtv);
	dsvHandle_ = DescriptorAllocator::Allocate(DescriptorUsage::Dsv);
	if(!rtvHandle_.IsValid() || !dsvHandle_.IsValid()) {
		ReleaseDescriptors();
		return false;
	}

	auto device = GraphicsGroup::GetInstance()->GetDevice();
	if(!device) {
		ReleaseDescriptors();
		return false;
	}

	renderTarget_ = std::make_unique<OffscreenRenderTarget>();
	renderTarget_->Initialize(device.Get(), width, height, format, rtvHandle_, dsvHandle_);
	renderTarget_->SetRenderTargetType(RenderTargetType::Offscreen);
	renderTarget_->SetClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	sprite_ = std::make_unique<Sprite>(renderTarget_->GetSRV());
	sprite_->Initialize({0.0f, 0.0f}, {static_cast<float>(width), static_cast<float>(height)});
	return true;
}

void RenderTextureSprite::Resize(uint32_t width, uint32_t height) {
	if(!renderTarget_ || width == 0 || height == 0 || recording3D_) return;
	renderTarget_->Resize(width, height);
	sprite_->SetTextureHandle(renderTarget_->GetSRV());
	sprite_->SetSize({static_cast<float>(width), static_cast<float>(height)});
}

void RenderTextureSprite::Begin3D(ID3D12GraphicsCommandList* commandList) {
	if(!renderTarget_ || !commandList || recording3D_) return;
	renderTarget_->TransitionTo(commandList, D3D12_RESOURCE_STATE_RENDER_TARGET);
	renderTarget_->TransitionDepthTo(commandList, D3D12_RESOURCE_STATE_DEPTH_WRITE);
	renderTarget_->Clear(commandList);
	renderTarget_->SetRenderTarget(commandList);
	recording3D_ = true;
}

void RenderTextureSprite::End3D(ID3D12GraphicsCommandList* commandList) {
	if(!renderTarget_ || !commandList || !recording3D_) return;
	renderTarget_->TransitionTo(commandList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
	recording3D_ = false;
}

void RenderTextureSprite::SetClearColor(float r, float g, float b, float a) {
	if(renderTarget_) renderTarget_->SetClearColor(r, g, b, a);
}

void RenderTextureSprite::ReleaseDescriptors() {
	if(rtvHandle_.IsValid()) DescriptorAllocator::Free(DescriptorUsage::Rtv, rtvHandle_);
	if(dsvHandle_.IsValid()) DescriptorAllocator::Free(DescriptorUsage::Dsv, dsvHandle_);
	rtvHandle_ = {};
	dsvHandle_ = {};
}
