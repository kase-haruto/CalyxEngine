#pragma once

#include <Engine/Graphics/Pipeline/PipelineDesc/Input/VertexLayout.h>
#include <Engine/Graphics/Material.h>

#include <vector>


struct MeshData{
	std::vector<VertexPosUvN> vertices;
	std::vector<uint32_t> indices;

	MaterialData material;
};