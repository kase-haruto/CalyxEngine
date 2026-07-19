#pragma once

#include <cstdint>

namespace CalyxEngine {

	enum class ParticleModuleStage : uint8_t {
		Emitter,
		Spawn,
		Initialize,
		Update,
		Render,
		Event,
	};

} // namespace CalyxEngine
