#pragma once
/* ========================================================================
/*	include space
/* ===================================================================== */
#include <Engine/Graphics/Buffer/DxStructuredBuffer.h>
#include <Engine/Application/Effects/Particle/FxUnit.h>
#include <Engine/Application/Effects/Particle/Parm/FxParm.h>
#include <Engine/Application/Effects/Particle/Emitter/BaseEmitter.h>
struct Vector3;

class GpuFxEmitter :
	public BaseEmitter {

	struct EmitterParam {
		float deltaTime;
		Vector3 acceleration = Vector3(0, -9.8f, 0);
	};

public:
	//===================================================================*/
	//					public methods
	//===================================================================*/
	GpuFxEmitter();
	~GpuFxEmitter();

	void Update(float deltaTime)override;
	void Dispatch(ID3D12GraphicsCommandList* cmdList);
	void TransferParticleDataToGPU()override;

	//--------- accessor -----------------------------------------------------
	//setter
	void SetPosition(const Vector3& pos);
	//getter
	const DxStructuredBuffer<ParticleCS>& GetParticleBuffer() const { return particleBuffer_; }

private:
	//===================================================================*/
	//					private methods
	//===================================================================*/
	Vector3 position_ = { 0, 0, 0 };

	// エフェクトのパラメータ
	FxParam<Vector3> scale_;
	FxParam<Vector3> velocity_;
	FxParam<float> lifetime_;

	static constexpr uint32_t kMaxParticles = 1024;
	DxStructuredBuffer<ParticleCS> particleBuffer_;
	EmitterParam emitParm_;
	DxConstantBuffer<EmitterParam> paramBuffer_;
};

