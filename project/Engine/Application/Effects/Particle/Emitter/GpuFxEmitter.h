#pragma once
#include <Engine/Application/Effects/Particle/Emitter/BaseEmitter.h>
#include <Engine/Application/Effects/Particle/FxUnit.h>
#include <Engine/Application/Effects/Particle/Parm/FxParm.h>
#include <Engine/Graphics/Buffer/DxConstantBuffer.h>
#include <Engine/Graphics/Buffer/DxStructuredBuffer.h>

struct CalyxMath::Vector3;
namespace CalyxEffect {

	/*-----------------------------------------------------------------------------------------
	 * GpuFxEmitter
	 * - GPUベースのパーティクルエミッタクラス
	 * - ComputeShaderを使用した大量パーティクルのシミュレーションを担当
	 *---------------------------------------------------------------------------------------*/
	class GpuFxEmitter
		: public BaseEmitter {
		/* ========================================================================
		/*		sutructs
		/* ===================================================================== */
		struct EmitterParam {
			float	deltaTime	 = 0.f;
			CalyxMath::Vector3 acceleration = CalyxMath::Vector3(0, 0, 0);
		};

		struct PerFrame {
			float time;
			float deltaTime;
		};

		struct EmitterSphere {
			CalyxMath::Vector3	 translate;
			float	 radius;
			uint32_t count;
			float	 frequency;
			float	 frequencyTime;
			uint32_t emit;
		};

	public:
		// 最大パーティクル数
		static constexpr uint32_t kMaxParticles = 1048576 * 4;

		GpuFxEmitter() = default;
		~GpuFxEmitter()override;

		void Initialize();
		void Update(float dt) override;
		void ShowGui();

		void ApplyConfigFrom(const EmitterConfig& config) override;
		void ExtractConfigTo(EmitterConfig& config) const override;

		//--------- Dispatch -----------------------------------------------------
		void DispatchInitialize(ID3D12GraphicsCommandList* cmd);
		void DispatchEmit(ID3D12GraphicsCommandList* cmd);
		void DispatchUpdate(ID3D12GraphicsCommandList* cmd);

		/// <summary>
		/// gpuにパラメータ転送
		/// </summary>
		void TransferParticleDataToGPU() override {}

		//--------- Accessor -----------------------------------------------------
		// getter
		D3D12_GPU_DESCRIPTOR_HANDLE GetParticleSrv() const;

		// setter
		void SetPosition(const CalyxMath::Vector3& pos) { position_ = pos; }

	private:
		CalyxMath::Vector3 position_{0, 0, 0};
		bool	isInitialized = false;

		// SBuff
		DxStructuredBuffer<ParticleCS> particleBuffer_; // UAV+SRV
		DxStructuredBuffer<int>		   freeListIndexBuffer_;
		DxStructuredBuffer<int>		   freeListBuffer_;

		// CBuff
		DxConstantBuffer<EmitterParam>	paramBuffer_;
		DxConstantBuffer<EmitterSphere> emitterParamBuf_;
		DxConstantBuffer<PerFrame>		perFrameBuffer_;

		// parm
		EmitterParam  emitParam_{};
		EmitterSphere emitterData_;
		PerFrame	  perFrame_;
	};
}