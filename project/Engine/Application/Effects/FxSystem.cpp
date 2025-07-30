#include "FxSystem.h"

#include <Engine/Application/Effects/Particle/FxUnit.h>
#include <Engine/Application/Effects/Particle/Object/ParticleSystemObject.h>


FxSystem::FxSystem() {
	particleRenderer_ = std::make_unique<ParticleRenderer>();

	connAdd_ = EventBus::Subscribe<ObjectAdded>(
		[this](const ObjectAdded& e) {
		if (auto fx = std::dynamic_pointer_cast<ParticleSystemObject>(e.sp))
			AddEmitter(fx);
	});

	connRem_ = EventBus::Subscribe<ObjectRemoved>(
		[this](const ObjectRemoved& e) {
		if (auto fx = std::dynamic_pointer_cast<ParticleSystemObject>(e.sp)) {
			RemoveEmitter(fx.get());
		}
	});
}

FxSystem::~FxSystem() {
	cpuEmitters_.clear();
	gpuEmitters_.clear();
}

void FxSystem::AddEmitter(const std::shared_ptr<BaseEmitter>& emitter) {
	if (auto cpu = std::dynamic_pointer_cast<FxEmitter>(emitter)) {
		cpuEmitters_.push_back(cpu);
	} else if (auto gpu = std::dynamic_pointer_cast<GpuFxEmitter>(emitter)) {
		gpuEmitters_.push_back(gpu);
	}
}

void FxSystem::RemoveEmitter(BaseEmitter* emitter) {
	auto removePredicate = [emitter](const auto& wp) {
		auto sp = wp.lock();
		return sp && sp.get() == emitter;
	};

	std::erase_if(cpuEmitters_, removePredicate);
	std::erase_if(gpuEmitters_, removePredicate);
}

void FxSystem::SyncEmitters() {
	for (auto it = cpuEmitters_.begin(); it != cpuEmitters_.end();) {
		if (auto emitter = it->lock()) {
			emitter->TransferParticleDataToGPU();
			++it;
		} else {
			it = cpuEmitters_.erase(it);
		}
	}

	// GPU emitter: 同様にデータを送る
	for (auto it = gpuEmitters_.begin(); it != gpuEmitters_.end();) {
		if (auto emitter = it->lock()) {
			emitter->TransferParticleDataToGPU();
			++it;
		} else {
			it = gpuEmitters_.erase(it);
		}
	}
}

void FxSystem::DispatchEmitters(ID3D12GraphicsCommandList* cmdList) {
	for (auto it = gpuEmitters_.begin(); it != gpuEmitters_.end();) {
		if (auto emitter = it->lock()) {
			emitter->Dispatch(cmdList);
		}
	}
}

void FxSystem::Render(PipelineService* pso, ID3D12GraphicsCommandList* cmd) {
	std::vector<std::shared_ptr<FxEmitter>> active;
	for (auto it = cpuEmitters_.begin(); it != cpuEmitters_.end();) {
		if (auto sp = it->lock()) {
			active.push_back(sp);
			++it;
		} else {
			it = cpuEmitters_.erase(it);
		}
	}
	particleRenderer_->Render(active, pso, cmd);
}

void FxSystem::Clear() {
	cpuEmitters_.clear();
	gpuEmitters_.clear();
}