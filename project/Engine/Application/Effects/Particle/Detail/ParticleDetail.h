
#pragma once
#include <Engine/Graphics/Pipeline/PipelineDesc/Input/VertexLayout.h>

/// <summary>
/// particleCBデータ
/// </summary>
struct ParticleConstantData {
	Vector3 position;
	Vector3 scale;
	Vector4 color;
	float rotation = 0.0f;
};
