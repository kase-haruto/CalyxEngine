
#pragma once
#include <Engine/Graphics/Pipeline/PipelineDesc/Input/VertexLayout.h>

namespace CxEffect {
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
