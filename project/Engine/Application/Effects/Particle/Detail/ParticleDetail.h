
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
		CalyxEngine::Vector3 rotation{0.0f,0.0f,0.0f};
		CalyxEngine::Vector3 alignDirection{0.0f,0.0f,0.0f};
		uint32_t             alignToDirection = 0;
	};
}
