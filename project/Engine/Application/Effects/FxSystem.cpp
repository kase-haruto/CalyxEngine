// FxSystem.cpp
#include "FxSystem.h"

#include <Engine/Application/Effects/Particle/Emitter/FxEmitter.h>
#include <Engine/Application/Effects/Particle/Emitter/GpuFxEmitter.h>
#include <Engine/Application/Effects/Particle/Object/ParticleSystemObject.h>

#include <Engine/Graphics/Pipeline/Service/PipelineService.h>
#include <Engine/Graphics/Context/GraphicsGroup.h>

/*===========================================================================*/
/*  ctor / dtor                                                              */
/*===========================================================================*/
FxSystem::FxSystem(){
	particleRenderer_ = std::make_unique<ParticleRenderer>();

	connAdd_ = EventBus::Subscribe<ObjectAdded>(
		[this] (const ObjectAdded& e){
			if (auto fx = std::dynamic_pointer_cast< ParticleSystemObject >(e.sp))
				AddEmitter(fx);
		});

	connRem_ = EventBus::Subscribe<ObjectRemoved>(
		[this] (const ObjectRemoved& e){
			if (auto fx = std::dynamic_pointer_cast< ParticleSystemObject >(e.sp))
				RemoveEmitter(fx.get());
		});
}

FxSystem::~FxSystem(){
	cpuEmitters_.clear();
	gpuEmitters_.clear();
}

/*===========================================================================*/
/*  追加 / 削除                                                              */
/*===========================================================================*/
void FxSystem::AddEmitter(const std::shared_ptr<BaseEmitter>& sp){
	if (auto cpu = std::dynamic_pointer_cast< FxEmitter >(sp)){
		cpuEmitters_.push_back(cpu);
	} else if (auto gpu = std::dynamic_pointer_cast< GpuFxEmitter >(sp)){
		gpuEmitters_.push_back(gpu);
	}
}

void FxSystem::RemoveEmitter(BaseEmitter* emitter){
	auto pred = [emitter] (const auto& wp){
		auto sp = wp.lock();
		return sp && sp.get() == emitter;
		};
	std::erase_if(cpuEmitters_, pred);
	std::erase_if(gpuEmitters_, pred);
}

/*===========================================================================*/
/*  毎フレーム同期 / ディスパッチ                                            */
/*===========================================================================*/
void FxSystem::SyncEmitters(){
	// ── CPU 側（Transform → GPU 転送） ──────────────────────
	for (auto it = cpuEmitters_.begin(); it != cpuEmitters_.end(); ){
		if (auto sp = it->lock()){
			sp->TransferParticleDataToGPU();
			++it;
		} else{
			it = cpuEmitters_.erase(it);
		}
	}

	// ── GPU 側（必要なら追加の CB などを転送） ─────────────────
	for (auto it = gpuEmitters_.begin(); it != gpuEmitters_.end(); ){
		if (auto sp = it->lock()){
			sp->TransferParticleDataToGPU();   // いまは空実装でも OK
			++it;
		} else{
			it = gpuEmitters_.erase(it);
		}
	}
}

void FxSystem::DispatchEmitters(PipelineService* psoService, ID3D12GraphicsCommandList* cmd){
	for (auto it = gpuEmitters_.begin(); it != gpuEmitters_.end(); ){
		if (auto sp = it->lock()){
			// 初期化パイプライン
			{
				auto psoInit = psoService->GetComputePipelineSet(PipelineTag::Compute::ParticleInitializeCompute);
				psoInit.SetCompute(cmd);
				sp->DispatchInitialize(cmd);
			}

			// Emit パイプライン
			{
				auto psoEmit = psoService->GetComputePipelineSet(PipelineTag::Compute::ParticleEmitCompute);
				psoEmit.SetCompute(cmd);
				sp->DispatchEmit(cmd);
			}

			// 更新
			{
				auto psoEmit = psoService->GetComputePipelineSet(PipelineTag::Compute::ParticleUpdateCompute);
				psoEmit.SetCompute(cmd);
				sp->DispatchUpdate(cmd);
			}

			++it;
		} else{
			it = gpuEmitters_.erase(it);
		}
	}
}

/*===========================================================================*/
/*  描画                                                                     */
/*===========================================================================*/
void FxSystem::Render(PipelineService* pso, ID3D12GraphicsCommandList* cmd){
	// ── アクティブな emitter を収集（無効 weak_ptr の掃除も兼ねる） ──
	std::vector<std::shared_ptr<FxEmitter>>   activeCpu;
	std::vector<std::shared_ptr<GpuFxEmitter>> activeGpu;

	for (auto it = cpuEmitters_.begin(); it != cpuEmitters_.end(); ){
		if (auto sp = it->lock()){
			activeCpu.push_back(sp);
			++it;
		} else{
			it = cpuEmitters_.erase(it);
		}
	}
	for (auto it = gpuEmitters_.begin(); it != gpuEmitters_.end(); ){
		if (auto sp = it->lock()){
			activeGpu.push_back(sp);
			++it;
		} else{
			it = gpuEmitters_.erase(it);
		}
	}

	// ── まとめて描画 ──────────────────────────────────────────
	particleRenderer_->Render(activeCpu, activeGpu, pso, cmd);
}

/*===========================================================================*/
/*  クリア                                                                   */
/*===========================================================================*/
void FxSystem::Clear(){
	cpuEmitters_.clear();
	gpuEmitters_.clear();
}
