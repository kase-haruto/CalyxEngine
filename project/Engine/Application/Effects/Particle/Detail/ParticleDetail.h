
#pragma once
#include <Engine/Graphics/Pipeline/PipelineDesc/Input/VertexLayout.h>

/// <summary>
/// particleCBデータ
/// </summary>
struct ParticleConstantData {
	CxMath::Vector3 position;
	CxMath::Vector3 scale;
	CxMath::Vector4 color;
	float rotation = 0.0f;
};
