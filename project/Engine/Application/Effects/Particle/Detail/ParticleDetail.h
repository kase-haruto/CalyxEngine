
#pragma once
#include <Engine/Graphics/Pipeline/PipelineDesc/Input/VertexLayout.h>

namespace CalyxEngine {
	/// <summary>
	/// particleCBデータ
	/// </summary>
	struct ParticleConstantData {
		CalyxEngine::Vector3 position;
		CalyxEngine::Vector3 scale;
		CalyxEngine::Vector4 color;
		float			rotation = 0.0f;
	};
}
