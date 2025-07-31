#pragma once
#include <Engine/Graphics/Buffer/DxStructuredBuffer.h>
#include <Engine/Graphics/Buffer/DxConstantBuffer.h>
#include <Engine/Application/Effects/Particle/FxUnit.h>
#include <Engine/Application/Effects/Particle/Parm/FxParm.h>
#include <Engine/Application/Effects/Particle/Emitter/BaseEmitter.h>

struct Vector3;

class GpuFxEmitter
	: public BaseEmitter{
	// ----------------------------------------------------------------
	struct EmitterParam{
		float  deltaTime = 0.f;
		Vector3 acceleration = Vector3(0, -9.8f, 0);
	};

public:
	static constexpr uint32_t kMaxParticles = 1024;

	GpuFxEmitter();
	~GpuFxEmitter();

	void Initialize();

	// 毎フレーム呼ぶ
	void Update(float dt) override;
	void Dispatch(ID3D12GraphicsCommandList* cmd);

	// 描画側で使う SRV
	D3D12_GPU_DESCRIPTOR_HANDLE GetParticleSrv() const{
		return particleBuffer_.GetGpuSrvHandle();
	}

	// 位置を変更したい場合
	void SetPosition(const Vector3& pos){ position_ = pos; }

	// ※ BaseEmitter::TransferParticleDataToGPU() は今回は不要
	void TransferParticleDataToGPU() override{}

private:
	Vector3 position_ {0,0,0};

	FxParam<Vector3> scale_ = FxParam<Vector3>::MakeConstant();
	FxParam<Vector3> velocity_ = FxParam<Vector3>::MakeRandom({-1,0,-1}, {1,0,1});
	FxParam<float>   lifetime_ = FxParam<float>::MakeRandom(1.f, 3.f);

	DxStructuredBuffer<ParticleCS> particleBuffer_;  // UAV+SRV
	DxConstantBuffer<EmitterParam> paramBuffer_;      // b0
	EmitterParam                    emitParam_ {};
};