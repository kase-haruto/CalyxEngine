
#pragma once
#include <Engine/Graphics/Pipeline/PipelineDesc/Input/VertexLayout.h>

namespace CalyxEffect {
	/// <summary>
	/// particleCBデータ
	/// </summary>
	struct ParticleConstantData {
		CalyxMath::Vector3 position;
		CalyxMath::Vector3 scale;
		CalyxMath::Vector4 color;
		float			rotation = 0.0f;
	};
}
