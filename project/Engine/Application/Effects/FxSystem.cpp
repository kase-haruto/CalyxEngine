#include "FxSystem.h"

#include <Engine/Application/Effects/Particle/Emitter/FxEmitter.h>
#include <Engine/Application/Effects/Particle/Emitter/GpuFxEmitter.h>
#include <Engine/Application/Effects/Particle/Object/ParticleSystemObject.h>
#include <Engine/Graphics/Context/GraphicsGroup.h>
#include <Engine/Graphics/Pipeline/Service/PipelineService.h>
#include <Engine/Objects/3D/Actor/SceneObject.h>

namespace CalyxEffect {
	/*===========================================================================*/
	/*  ctor / dtor                                                              */
	/*===========================================================================*/
	FxSystem::FxSystem() {
		particleRenderer_ = std::make_unique<ParticleRenderer>();

		// 追加イベント
		connAdd_ = EventBus::Subscribe<ObjectAdded>(
			[this](const ObjectAdded& e) {
				if(auto ps = std::dynamic_pointer_cast<ParticleSystemObject>(e.sp)) {
					auto emitter = ps->GetEmitter(); // std::shared_ptr<CalyxEffect::BaseEmitter> など
					if(emitter) {
						AddEmitter(emitter, ps->GetGuid());
					}
				}
			});

		// 削除イベント（SceneObject が消えたら GUID で emitter を外す）
		connRem_ = EventBus::Subscribe<ObjectRemoved>(
			[this](const ObjectRemoved& e) {
				RemoveEmitterByGuid(e.sp->GetGuid());
			});
	}

	FxSystem::~FxSystem() {
		cpuEmitters_.clear();
		gpuEmitters_.clear();
	}

	/*===========================================================================*/
	/*  追加 / 削除                                                              */
	/*===========================================================================*/

	void FxSystem::AddEmitter(const std::shared_ptr<CalyxEffect::BaseEmitter>& sp,
							  const Guid&									   ownerGuid) {
		if(!sp || !ownerGuid.isValid()) return;

		// CPU emitter?
		if(auto cpu = std::dynamic_pointer_cast<CalyxEffect::FxEmitter>(sp)) {
			// 重複登録チェック（同じ ownerGuid & emitter）
			auto it = std::find_if(cpuEmitters_.begin(), cpuEmitters_.end(),
								   [&](const CpuEmitterEntry& e) {
									   auto locked = e.emitter.lock();
									   return (e.ownerGuid == ownerGuid) &&
											  locked && (locked.get() == cpu.get());
								   });
			if(it == cpuEmitters_.end()) {
				CpuEmitterEntry entry;
				entry.ownerGuid = ownerGuid;
				entry.emitter	= cpu;
				cpuEmitters_.push_back(entry);
			}
			return;
		}

		// GPU emitter?
		if(auto gpu = std::dynamic_pointer_cast<CalyxEffect::GpuFxEmitter>(sp)) {
			auto it = std::find_if(gpuEmitters_.begin(), gpuEmitters_.end(),
								   [&](const GpuEmitterEntry& e) {
									   auto locked = e.emitter.lock();
									   return (e.ownerGuid == ownerGuid) &&
											  locked && (locked.get() == gpu.get());
								   });
			if(it == gpuEmitters_.end()) {
				GpuEmitterEntry entry;
				entry.ownerGuid = ownerGuid;
				entry.emitter	= gpu;
				gpuEmitters_.push_back(entry);
			}
			return;
		}

		// どちらでもなければ何もしない
	}

	void FxSystem::RemoveEmitter(CalyxEffect::BaseEmitter* emitter) {
		if(!emitter) return;

		// CPU emitter 側をポインタ一致で削除
		std::erase_if(cpuEmitters_,
					  [emitter](const CpuEmitterEntry& e) {
						  auto sp = e.emitter.lock();
						  return !sp || (sp.get() == emitter);
					  });

		// GPU emitter 側をポインタ一致で削除
		std::erase_if(gpuEmitters_,
					  [emitter](const GpuEmitterEntry& e) {
						  auto sp = e.emitter.lock();
						  return !sp || (sp.get() == emitter);
					  });
	}

	void FxSystem::RemoveEmitterByGuid(const Guid& id) {
		if(!id.isValid()) return;

		// ownerGuid が一致するものを削除（weak_ptr 失効もついでに掃除）
		std::erase_if(cpuEmitters_,
					  [&](const CpuEmitterEntry& e) {
						  if(!e.emitter.lock()) return true;
						  return (e.ownerGuid == id);
					  });

		std::erase_if(gpuEmitters_,
					  [&](const GpuEmitterEntry& e) {
						  if(!e.emitter.lock()) return true;
						  return (e.ownerGuid == id);
					  });
	}

	/*===========================================================================*/
	/*  毎フレーム同期 / ディスパッチ                                            */
	/*===========================================================================*/
	void FxSystem::SyncEmitters() {
		// CPU
		for(auto it = cpuEmitters_.begin(); it != cpuEmitters_.end();) {
			if(auto sp = it->emitter.lock()) {
				sp->TransferParticleDataToGPU();
				++it;
			} else {
				// weak_ptr が失効していたら削除
				it = cpuEmitters_.erase(it);
			}
		}

		// GPU
		for(auto it = gpuEmitters_.begin(); it != gpuEmitters_.end();) {
			if(auto sp = it->emitter.lock()) {
				sp->TransferParticleDataToGPU();
				++it;
			} else {
				it = gpuEmitters_.erase(it);
			}
		}
	}

	void FxSystem::DispatchEmitters(PipelineService*		   psoService,
									ID3D12GraphicsCommandList* cmd) {
		for(auto it = gpuEmitters_.begin(); it != gpuEmitters_.end();) {
			if(auto sp = it->emitter.lock()) {
				// 初期化
				{
					auto psoInit = psoService->GetComputePipelineSet(
						PipelineTag::Compute::ParticleInitializeCompute);
					psoInit.SetCompute(cmd);
					sp->DispatchInitialize(cmd);
				}
				// Emit
				{
					auto psoEmit = psoService->GetComputePipelineSet(
						PipelineTag::Compute::ParticleEmitCompute);
					psoEmit.SetCompute(cmd);
					sp->DispatchEmit(cmd);
				}
				// Update
				{
					auto psoUpd = psoService->GetComputePipelineSet(
						PipelineTag::Compute::ParticleUpdateCompute);
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
		std::vector<std::shared_ptr<CalyxEffect::FxEmitter>>	activeCpu;
		std::vector<std::shared_ptr<CalyxEffect::GpuFxEmitter>> activeGpu;

		for(auto it = cpuEmitters_.begin(); it != cpuEmitters_.end();) {
			if(auto sp = it->emitter.lock()) {
				activeCpu.push_back(sp);
				++it;
			} else {
				it = cpuEmitters_.erase(it);
			}
		}
		for(auto it = gpuEmitters_.begin(); it != gpuEmitters_.end();) {
			if(auto sp = it->emitter.lock()) {
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

} // namespace CalyxEffect