#pragma once

#include <d3d12.h>
#include <Engine/Foundation/Math/Matrix4x4.h>
#include <Engine/Graphics/Material.h>
#include <Engine/Graphics/RenderTarget/Detail/RenderTargetDetail.h>

namespace CalyxEngine {

	struct SpriteDrawData {
		D3D12_GPU_DESCRIPTOR_HANDLE texture{};
		CalyxEngine::Matrix4x4        wvp;
		Material2D                  material{};
		RenderTargetType            targetRT = RenderTargetType::BackBuffer;
	};

} // namespace CalyxEngine
