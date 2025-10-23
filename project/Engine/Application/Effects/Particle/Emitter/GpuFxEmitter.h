#pragma once

// engine
#include <Engine/Application/Effects/Particle/Emitter/BaseEmitter.h>
#include <Engine/Application/Effects/Particle/FxUnit.h>
#include <Engine/Application/Effects/Particle/Parm/FxParm.h>
#include <Engine/Graphics/Buffer/DxConstantBuffer.h>
#include <Engine/Graphics/Buffer/DxStructuredBuffer.h>

// fwd
struct Vector3;

/// <summary>
/// gupuパーティクル発生emitter
/// </summary>
class GpuFxEmitter
	: public BaseEmitter {

	struct EmitterParam {
		float	deltaTime	 = 0.f;
		Vector3 acceleration = Vector3(0, 0, 0);
	};

	struct PerFrame {
		float time;
		float deltaTime;
	};

	struct EmitterSphere {
		Vector3	 translate;
		float	 radius;
		uint32_t count;
		float	 frequency;
		float	 frequencyTime;
		uint32_t emit;
	};

public:
	/// <summary>
	/// 最大数
	/// </summary>
	static constexpr uint32_t kMaxParticles = 1048576 * 4;

	GpuFxEmitter() = default;
	~GpuFxEmitter();

	void Initialize();
	void Update(float dt) override;
	void ShowGui();

	// dispatch
	void DispatchInitialize(ID3D12GraphicsCommandList* cmd);
	void DispatchEmit(ID3D12GraphicsCommandList* cmd);
	void DispatchUpdate(ID3D12GraphicsCommandList* cmd);

	// データ転送
	void TransferParticleDataToGPU() override {}

	// 描画側で使う SRV
	D3D12_GPU_DESCRIPTOR_HANDLE GetParticleSrv() const;

	// setter
	void SetPosition(const Vector3& pos) { position_ = pos; }
private:
	Vector3 position_{0, 0, 0};
	bool	isInitialized = false;

	// Structuerdbuffer
	DxStructuredBuffer<ParticleCS> particleBuffer_; // UAV+SRV
	DxStructuredBuffer<int>		   freeListIndexBuffer_;
	DxStructuredBuffer<int>		   freeListBuffer_;

	// CBV
	DxConstantBuffer<EmitterParam>	paramBuffer_;
	DxConstantBuffer<EmitterSphere> emitterParamBuf_;
	DxConstantBuffer<PerFrame>		perFrameBuffer_;

	// parm
	EmitterParam  emitParam_{};
	EmitterSphere emitterData_;
	PerFrame	  perFrame_;
};