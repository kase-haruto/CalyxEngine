#pragma once
/* ========================================================================
/*	include space
/* ===================================================================== */
// engine
#include <Engine/Application/Effects/Particle/Emitter/FxEmitter.h>
#include <Engine/Application/Effects/Particle/Emitter/GpuFxEmitter.h>
#include <Engine/Objects/ConfigurableObject/ConfigurableObject.h>
#include <Engine/Renderer/Particle/ParticleRenderer.h>
#include <Engine/System/Event/EventBus.h>

// c++ 
#include <memory>

/* ========================================================================
/*	effect system
/* ===================================================================== */
class FxSystem {
public:
	//===================================================================*/
	//					public func
	//===================================================================*/
	FxSystem();
	~FxSystem() ;
	void AddEmitter(const std::shared_ptr<BaseEmitter>& emitter);
	void RemoveEmitter(BaseEmitter* emitter);
	void SyncEmitters();
	void DispatchEmitters(ID3D12GraphicsCommandList* cmdList);
	void Render(class PipelineService*, ID3D12GraphicsCommandList*);
	void Clear();
private:
	//===================================================================*/
	//					private variable
	//===================================================================*/
	std::vector<std::weak_ptr<FxEmitter>> cpuEmitters_;
	std::vector<std::weak_ptr<GpuFxEmitter>> gpuEmitters_;
	std::unique_ptr<ParticleRenderer> particleRenderer_;

	EventBus::Connection connAdd_;
	EventBus::Connection connRem_;
};