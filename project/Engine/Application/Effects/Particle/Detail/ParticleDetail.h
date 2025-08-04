
#pragma once
#include <Engine/Graphics/Pipeline/PipelineDesc/Input/VertexLayout.h>
#include <Engine/Assets/Model/ModelData.h>

#include <memory>
#include <wrl/client.h>

struct ParticleConstantData {
	Vector3 position = Vector3::Zero();
	Vector3 scale= Vector3(1.0f);
	Vector4 color= Vector4({1.0f,1.0f,1.0f});
};
