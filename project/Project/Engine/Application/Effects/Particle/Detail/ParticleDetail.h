
#pragma once
#include <Engine/Graphics/Pipeline/PipelineDesc/Input/VertexLayout.h>
#include <Engine/Assets/Model/ModelData.h>

#include <memory>
#include <wrl/client.h>

struct ParticleConstantData {
	Vector3 position;
	Vector3 scale;
	Vector4 color;
};
