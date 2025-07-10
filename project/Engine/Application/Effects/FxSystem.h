#pragma once
/* ========================================================================
/*	include space
/* ===================================================================== */
// engine
#include <Engine/Application/Effects/Particle/Emitter/FxEmitter.h>
#include <Engine/Objects/ConfigurableObject/ConfigurableObject.h>
#include <Engine/Renderer/Particle/ParticleRenderer.h>

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
	void AddEmitter(const std::shared_ptr<FxEmitter>& emitter);
	void RemoveEmitter(const std::shared_ptr<FxEmitter>& emitter);
	void Update();
	void Render(class PipelineService*, ID3D12GraphicsCommandList*);
	void Clear();
private:
	//===================================================================*/
	//					private variable
	//===================================================================*/
	std::vector<std::weak_ptr<FxEmitter>> emitters_;
	std::unique_ptr<ParticleRenderer> particleRenderer_;
};