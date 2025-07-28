#pragma once
/* ========================================================================
/*	include space
/* ===================================================================== */
#include <Engine/Graphics/Buffer/DxStructuredBuffer.h>
#include <Engine/Application/Effects/Particle/FxUnit.h>
#include <Engine/Application/Effects/Particle/Parm/FxParm.h>

struct Vector3;

class GpuFxEmitter{
public:
	//===================================================================*/
	//					public methods
	//===================================================================*/
	GpuFxEmitter() = default;
	~GpuFxEmitter() = default;

	void Initialize(ID3D12Device* device);
	void Update(ID3D12GraphicsCommandList* cmdList, float deltaTime);
	void Dispatch(ID3D12GraphicsCommandList* cmdList);

	//--------- accessor -----------------------------------------------------
	//setter
	void SetPosition(const Vector3& pos);

	//getter
	const DxStructuredBuffer<ParticleCS>& GetParticleBuffer() const{ return particleBuffer_; }

private:
	//===================================================================*/
	//					private methods
	//===================================================================*/
	DxStructuredBuffer<ParticleCS> particleBuffer_;
	Vector3 emitterPosition_ = {0, 0, 0};
	float currentTime_ = 0.0f;

	// エフェクトのパラメータ
	FxParam<Vector3> scale_;
	FxParam<Vector3> velocity_;
	FxParam<float> lifetime_;

	static constexpr uint32_t kMaxParticles = 1024;
};

