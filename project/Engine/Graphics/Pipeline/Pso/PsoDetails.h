#pragma once

#include <d3d12.h>
#include <cstdint>

struct PipelineSet {
	ID3D12PipelineState* pipelineState;
	ID3D12RootSignature* rootSignature;

	void SetCommand(ID3D12GraphicsCommandList* cmd) const {
		cmd->SetPipelineState(pipelineState);
		cmd->SetGraphicsRootSignature(rootSignature);
	}

	void SetCompute(ID3D12GraphicsCommandList* cmd) const {
		cmd->SetPipelineState(pipelineState);
		cmd->SetComputeRootSignature(rootSignature);
	}
};

namespace PipelineTag {
	enum class Object :std::uint16_t {
		Object3d,
		SkinningObject3D,
		WireframeObject3D,
		WireframeSkinnedObject3D,
		ShadowStatic,
		ShadowSkinned,
		PickingObject3D,
		PickingSkinned,
		Object2d,
		Particle,
		GpuParticle,
		Line,
		Effect,
		Skybox,

		count,
	};

	enum class PostProcess :std::uint16_t {
		CopyImage,
		GrayScale,
		RadialBlur,
		ChromaticAberration,
		Vignette,
		CRT,

		Count
	};

	enum class Compute : std::uint16_t {
		ParticleInitializeCompute = 0,
		ParticleEmitCompute,
		ParticleUpdateCompute,
		kComputeCount
	};
} // namespace PipelineTag