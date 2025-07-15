#include "FxSystem.h"

#include <Engine/Application/Effects/Particle/FxUnit.h>


FxSystem::FxSystem(){
	particleRenderer_ = std::make_unique<ParticleRenderer>();
}

FxSystem::~FxSystem(){
	emitters_.clear();
}

void FxSystem::AddEmitter(const std::shared_ptr<FxEmitter>& emitter){
	emitters_.push_back(emitter);
}

void FxSystem::RemoveEmitter(const std::shared_ptr<FxEmitter>& emitter){
	emitters_.erase(
		std::remove_if(emitters_.begin(), emitters_.end(),
		[&] (const std::weak_ptr<FxEmitter>& wptr){
			auto sptr = wptr.lock();
			return !sptr || sptr == emitter;
		}),
		emitters_.end()
	);
}

void FxSystem::SyncEmitters(){
	for (auto it = emitters_.begin(); it != emitters_.end(); ){
		if (auto emitter = it->lock()){
			emitter->TransferParticleDataToGPU();   // 計算済みデータを GPU へ
			++it;
		} else{
			it = emitters_.erase(it);           // 弱参照切れを削除
		}
	}
}

void FxSystem::Render(PipelineService* pso, ID3D12GraphicsCommandList* cmd){
	std::vector<std::shared_ptr<FxEmitter>> aliveEmitters;
	for (auto it = emitters_.begin(); it != emitters_.end();){
		if (auto e = it->lock()){
			aliveEmitters.push_back(e);
			++it;
		} else{
			it = emitters_.erase(it);
		}
	}
	particleRenderer_->Render(aliveEmitters, pso, cmd);
}

void FxSystem::Clear(){
	emitters_.clear();
}