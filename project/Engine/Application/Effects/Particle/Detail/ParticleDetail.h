
#pragma once
#include <Engine/Graphics/Pipeline/PipelineDesc/Input/VertexLayout.h>
#include <cstdint>

namespace CalyxEngine {
	/// <summary>
	/// particleCBデータ
	/// </summary>
	struct ParticleConstantData {
		CalyxEngine::Vector3 position;
		CalyxEngine::Vector3 scale;
		CalyxEngine::Vector4 color;
		CalyxEngine::Vector4 vertexColor;
		CalyxEngine::Vector4 flipbookScaleOffset{1.0f,1.0f,0.0f,0.0f}; //< xy: frame scale, zw: frame offset
		CalyxEngine::Vector4 emissiveColor{1.0f,1.0f,1.0f,1.0f};
		float                emissiveIntensity = 0.0f;
		CalyxEngine::Vector3 emissivePadding{}; //< StructuredBufferのC++/HLSL strideを16-byte境界へ揃える
		CalyxEngine::Vector3 rotation{0.0f,0.0f,0.0f};
		CalyxEngine::Vector3 alignDirection{0.0f,0.0f,0.0f};
		uint32_t             alignToDirection = 0;
	};
}
