#pragma once

#include <d3d12.h>
#include <cstdint>

struct PipelineSet{
	ID3D12PipelineState* pipelineState;
	ID3D12RootSignature* rootSignature;

	void SetCommand(ID3D12GraphicsCommandList* cmd) const{
		cmd->SetPipelineState(pipelineState);
		cmd->SetGraphicsRootSignature(rootSignature);
	}
};

namespace PipelineTag{
	enum class Object :std::uint16_t{
		Object3d,
		SkinningObject3D,
		Object2d,
		Particle,
		Line,
		Effect,
		Skybox,

		count
	};

	enum class PostProcess :std::uint16_t{
		CopyImage,
		GrayScale,
		RadialBlur,
		ChromaticAberration,
		Vignette,

		Count
	};

} // namespace PipelineTag