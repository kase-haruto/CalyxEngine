#include "FxSystem.h"

#include <Engine/Application/Effects/Particle/FxUnit.h>
#include <Engine/Application/Effects/Particle/Object/ParticleSystemObject.h>


FxSystem::FxSystem(){
	particleRenderer_ = std::make_unique<ParticleRenderer>();

	connAdd_ = EventBus::Subscribe<ObjectAdded>(
		[this] (const ObjectAdded& e){
			if (auto fx = std::dynamic_pointer_cast< ParticleSystemObject >(e.sp))
				AddEmitter(fx);
		});

	connRem_ = EventBus::Subscribe<ObjectRemoved>(
		[this] (const ObjectRemoved& e){
			if (auto fx = std::dynamic_pointer_cast< ParticleSystemObject >(e.sp)){
				RemoveEmitter(fx.get());
			}
		});
}

FxSystem::~FxSystem(){
	emitters_.clear();
}

void FxSystem::AddEmitter(const std::shared_ptr<FxEmitter>& emitter){
	emitters_.push_back(emitter);
}

void FxSystem::RemoveEmitter(FxEmitter* emitter){
	emitters_.erase(
		std::remove_if(emitters_.begin(), emitters_.end(),
		[emitter] (const std::weak_ptr<FxEmitter>& wp){
			auto sp = wp.lock(); // weak_ptr → shared_ptr に変換
			return sp && sp.get() == emitter; // ポインタ比較
		}),
		emitters_.end());
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