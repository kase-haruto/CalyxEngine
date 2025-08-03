#include "BaseEmitter.h"

#include <iostream>

void BaseEmitter::TransferParticleDataToGPU() {
	std::vector<ParticleConstantData> gpuUnits;
	for (const auto& fx : units_) {
		if (fx.alive) {
			gpuUnits.push_back({ fx.position, fx.scale, fx.color });
		}
	}
	std::cout << "[Transfer] count: " << gpuUnits.size() << std::endl;

	if (!gpuUnits.empty()) {
		instanceBuffer_.TransferVectorData(gpuUnits);
	}
}