#pragma once
/* ========================================================================
/*	include space
/* ===================================================================== */
//engine
#include <Engine/Graphics/RenderTarget/Interface/IRenderTarget.h>

//c++
#include <d3d12.h>
#include <wrl.h>
#include <string>

class IPostEffectPass{
public:
	virtual ~IPostEffectPass() = default;

	virtual void Apply(ID3D12GraphicsCommandList* cmd,
					   D3D12_GPU_DESCRIPTOR_HANDLE inputSRV,
					   IRenderTarget* outputRT) = 0;

	virtual void ShowImGui() {}

	virtual const std::string GetName() const = 0;
};
