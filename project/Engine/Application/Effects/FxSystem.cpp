#include "FxSystem.h"

#include <Engine/Application/Effects/Particle/Emitter/FxEmitter.h>
#include <Engine/Application/Effects/Particle/Emitter/GpuFxEmitter.h>
#include <Engine/Application/Effects/Particle/Object/ParticleSystemObject.h>
#include <Engine/Graphics/Context/GraphicsGroup.h>
#include <Engine/Graphics/Pipeline/Service/PipelineService.h>
#include <Engine/Objects/3D/Actor/SceneObject.h>

/*===========================================================================*/
/*  helpers                                                                  */
/*===========================================================================*/

// weak_ptr コンテナにguidがあるか判定
template <class T>
static bool ContainsGuid(const std::vector<std::weak_ptr<T>>& vec, const Guid& id) {
	for(auto const& w : vec) {
		if(auto sp = w.lock()) {
			if(auto so = std::dynamic_pointer_cast<SceneObject>(sp)) {
				if(so->GetGuid() == id) return true;
			}
		}
	}
	return false;
}

// weak_ptr コンテナから削除（
template <class T>
static void EraseByGuid(std::vector<std::weak_ptr<T>>& vec, const Guid& id) {
	auto pred = [&](const std::weak_ptr<T>& w) {
		auto sp = w.lock();
		if(!sp) return true; // 失効は掃除
		if(auto so = std::dynamic_pointer_cast<SceneObject>(sp)) {
			return (so->GetGuid() == id);
		}
		return false;
	};
	std::erase_if(vec, pred);
}

/*===========================================================================*/
/*  ctor / dtor                                                              */
/*===========================================================================*/
FxSystem::FxSystem() {
	particleRenderer_ = std::make_unique<ParticleRenderer>();

	// 追加イベント
	connAdd_ = EventBus::Subscribe<ObjectAdded>(
		[this](const ObjectAdded& e) {
			if(auto fx = std::dynamic_pointer_cast<ParticleSystemObject>(e.sp)) {
				AddEmitter(fx);
			}
		});

	// 削除イベント
	connRem_ = EventBus::Subscribe<ObjectRemoved>(
		[this](const ObjectRemoved& e) { RemoveEmitterByGuid(e.sp->GetGuid()); });
}

FxSystem::~FxSystem() {
	cpuEmitters_.clear();
	gpuEmitters_.clear();
}

/*===========================================================================*/
/*  追加 / 削除                                                              */
/*===========================================================================*/

void FxSystem::AddEmitter(const std::shared_ptr<BaseEmitter>& sp) {
	if(!sp) return;

	Guid gid{};
	if(auto so = std::dynamic_pointer_cast<SceneObject>(sp)) {
		gid = so->GetGuid();
	}

	if(auto cpu = std::dynamic_pointer_cast<FxEmitter>(sp)) {
		if(gid.isValid() && ContainsGuid(cpuEmitters_, gid)) return;
		cpuEmitters_.push_back(cpu);
		return;
	}
	if(auto gpu = std::dynamic_pointer_cast<GpuFxEmitter>(sp)) {
		if(gid.isValid() && ContainsGuid(gpuEmitters_, gid)) return;
		gpuEmitters_.push_back(gpu);
		return;
	}
}

void FxSystem::RemoveEmitter(BaseEmitter* emitter) {
	if(!emitter) return;

	if(auto so = dynamic_cast<SceneObject*>(emitter)) {
		auto gid = so->GetGuid();
		if(gid.isValid()) {
			RemoveEmitterByGuid(gid);
			return;
		}
	}

	// フォールバック
	auto pred = [emitter](const auto& wp) {
		auto sp = wp.lock();
		return sp && (sp.get() == emitter);
	};
	std::erase_if(cpuEmitters_, pred);
	std::erase_if(gpuEmitters_, pred);
}

void FxSystem::RemoveEmitterByGuid(const Guid& id) {
	if(!id.isValid()) return;
	EraseByGuid(cpuEmitters_, id);
	EraseByGuid(gpuEmitters_, id);
}

/*===========================================================================*/
/*  毎フレーム同期 / ディスパッチ                                            */
/*===========================================================================*/
void FxSystem::SyncEmitters() {
	for(auto it = cpuEmitters_.begin(); it != cpuEmitters_.end();) {
		if(auto sp = it->lock()) {
			sp->TransferParticleDataToGPU();
			++it;
		} else {
			it = cpuEmitters_.erase(it);
		}
	}

	for(auto it = gpuEmitters_.begin(); it != gpuEmitters_.end();) {
		if(auto sp = it->lock()) {
			sp->TransferParticleDataToGPU();
			++it;
		} else {
			it = gpuEmitters_.erase(it);
		}
	}
}

void FxSystem::DispatchEmitters(PipelineService* psoService, ID3D12GraphicsCommandList* cmd) {
	for(auto it = gpuEmitters_.begin(); it != gpuEmitters_.end();) {
		if(auto sp = it->lock()) {
			// 初期化
			{
				auto psoInit = psoService->GetComputePipelineSet(PipelineTag::Compute::ParticleInitializeCompute);
				psoInit.SetCompute(cmd);
				sp->DispatchInitialize(cmd);
			}
			// Emit
			{
				auto psoEmit = psoService->GetComputePipelineSet(PipelineTag::Compute::ParticleEmitCompute);
				psoEmit.SetCompute(cmd);
				sp->DispatchEmit(cmd);
			}
			// Update
			{
				auto psoUpd = psoService->GetComputePipelineSet(PipelineTag::Compute::ParticleUpdateCompute);
				psoUpd.SetCompute(cmd);
				sp->DispatchUpdate(cmd);
			}
			++it;
		} else {
			it = gpuEmitters_.erase(it);
		}
	}
}

/*===========================================================================*/
/*  描画                                                                     */
/*===========================================================================*/
void FxSystem::Render(PipelineService* pso, ID3D12GraphicsCommandList* cmd) {
	std::vector<std::shared_ptr<FxEmitter>>	   activeCpu;
	std::vector<std::shared_ptr<GpuFxEmitter>> activeGpu;

	for(auto it = cpuEmitters_.begin(); it != cpuEmitters_.end();) {
		if(auto sp = it->lock()) {
			activeCpu.push_back(sp);
			++it;
		} else {
			it = cpuEmitters_.erase(it);
		}
	}
	for(auto it = gpuEmitters_.begin(); it != gpuEmitters_.end();) {
		if(auto sp = it->lock()) {
			activeGpu.push_back(sp);
			++it;
		} else {
			it = gpuEmitters_.erase(it);
		}
	}

	particleRenderer_->Render(activeCpu, activeGpu, pso, cmd);
}

/*===========================================================================*/
/*  クリア                                                                   */
/*===========================================================================*/
void FxSystem::Clear() {
	cpuEmitters_.clear();
	gpuEmitters_.clear();
}
