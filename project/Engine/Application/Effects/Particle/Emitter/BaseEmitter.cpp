#include "BaseEmitter.h"

void BaseEmitter::TransferParticleDataToGPU() {
	std::vector<ParticleConstantData> gpuUnits;
	gpuUnits.clear();
	for (const auto& fx : units_) {
		if (fx.alive) {
			gpuUnits.push_back({ fx.position, fx.scale, fx.color });
		}
	}
	if (!gpuUnits.empty()) {

		instanceBuffer_.TransferVectorData(gpuUnits);
	}
}
