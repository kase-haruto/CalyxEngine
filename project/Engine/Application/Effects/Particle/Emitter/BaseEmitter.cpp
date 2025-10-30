#include "BaseEmitter.h"

#include <iostream>

BaseEmitter::BaseEmitter() =default;

void BaseEmitter::TransferParticleDataToGPU(){
	if (units_.empty()) return;
	std::vector<ParticleConstantData> gpuUnits;
	for (const auto& fx : units_){
		if (fx.alive){
			gpuUnits.push_back({fx.position, fx.scale, fx.color});
		}
	}
	if (!gpuUnits.empty()){
		instanceBuffer_.TransferVectorData(gpuUnits);
	}
}