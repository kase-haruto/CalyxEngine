#pragma once
#include <Engine/Graphics/Pipeline/BlendMode/BlendMode.h>
#include "PipelineType.h"

#include <cstdint>
#include <functional>
#include <optional>

struct PipelineKey{
	PipelineType pipelineType{};
	std::optional<BlendMode> blendMode{}; // nullopt = blend 無関係

	static PipelineKey WithBlend(PipelineType type, BlendMode mode){
		return PipelineKey{ type, mode };
	}

	static PipelineKey NoBlend(PipelineType type){
		return PipelineKey{ type, std::nullopt };
	}

	bool operator==(const PipelineKey& other) const{
		return (pipelineType == other.pipelineType) && (blendMode == other.blendMode);
	}
};

namespace std{
	template<>
	struct hash<PipelineKey>{
		size_t operator()(const PipelineKey& key) const{
			auto h1 = std::hash<size_t>{}(static_cast<size_t>(key.pipelineType));
			// blendMode がある場合のみ混ぜる。無い場合は番兵を混ぜる
			auto h2 = key.blendMode
				? std::hash<size_t>{}(static_cast<size_t>(*key.blendMode))
				: std::hash<size_t>{}(0x9E3779B9u);

			// hash combine
			return h1 ^ (h2 + 0x9E3779B97F4A7C15ull + (h1 << 6) + (h1 >> 2));
		}
	};
}