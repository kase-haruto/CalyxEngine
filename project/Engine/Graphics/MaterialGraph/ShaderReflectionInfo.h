#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace CalyxEngine {
	enum class ShaderResourceKind : int32_t {
		Unknown,
		CBuffer,
		Texture,
		Sampler,
		UAV,
		StructuredBuffer,
		RaytracingAccelerationStructure,
	};

	struct ShaderResourceBinding {
		std::string name;
		ShaderResourceKind kind = ShaderResourceKind::Unknown;
		uint32_t bindPoint = 0;
		uint32_t bindCount = 0;
		uint32_t space = 0;
	};

	struct ShaderCBufferVariable {
		std::string name;
		uint32_t offset = 0;
		uint32_t size = 0;
	};

	struct ShaderCBufferLayout {
		std::string name;
		uint32_t size = 0;
		uint32_t bindPoint = 0;
		uint32_t space = 0;
		std::vector<ShaderCBufferVariable> variables;
	};

	struct ShaderReflectionInfo {
		std::string entryPoint;
		std::string profile;
		std::vector<ShaderResourceBinding> resources;
		std::vector<ShaderCBufferLayout> cbuffers;
	};
}
