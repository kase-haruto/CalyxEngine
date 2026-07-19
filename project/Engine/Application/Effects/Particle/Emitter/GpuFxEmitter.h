#pragma once
#include <Engine/Application/Effects/Particle/Emitter/BaseEmitter.h>
#include <Engine/Application/Effects/Particle/FxUnit.h>
#include <Engine/Application/Effects/Particle/Parm/FxParm.h>
#include <Engine/Graphics/Buffer/DxConstantBuffer.h>
#include <Engine/Graphics/Buffer/DxStructuredBuffer.h>
#include <Engine/Foundation/Math/Vector4.h>

struct CalyxEngine::Vector3;
namespace CalyxEngine {

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
			CalyxEngine::Vector3 acceleration = CalyxEngine::Vector3(0, 0, 0);
		};

		struct PerFrame {
			float time;
			float deltaTime;
		};

		struct EmitterSphere {
			CalyxEngine::Vector3	 translate;
			float	 radius;
			uint32_t count;
			float	 frequency;
			float	 frequencyTime;
			uint32_t emit;
			CalyxEngine::Vector3 scale;
			float	 lifeTime;
			CalyxEngine::Vector3 velocity;
			float	 angle;
			CalyxEngine::Vector4 color;
			CalyxEngine::Vector3 shapeSize;
			uint32_t shape;
			CalyxEngine::Vector4 rotation;
			CalyxEngine::Vector3 gravity;
			uint32_t gravityEnabled;
			CalyxEngine::Vector4 overLifeStart;
			CalyxEngine::Vector4 overLifeEnd;
			uint32_t overLifeTarget;
			uint32_t overLifeBlend;
			uint32_t overLifeEase;
			uint32_t overLifeEnabled;
			uint32_t overLifeClamp;
			uint32_t overLifeInvert;
			uint32_t sizeLifeEnabled;
			uint32_t sizeLifeGrowing;
			uint32_t sizeLifeEase;
			float _pad1;
		};

	public:
		// 最大パーティクル数
		static constexpr uint32_t kMaxParticles = 262144;

		GpuFxEmitter() = default;
		~GpuFxEmitter()override;

		void Initialize();
		void Update(float dt) override;
		void ShowGui() override;
		void DrawEmitterShape(const WorldTransform& tf) override;
		void DrawEmitterShapePreview(const WorldTransform& tf) override;

		void ApplyConfigFrom(const EmitterConfig& config) override;
		void ExtractConfigTo(EmitterConfig& config) const override;
		void Play() override;
		void Stop() override;
		void Reset() override;
		bool IsPlaying() const override { return isPlaying_; }
		bool IsDrawEnable() const override { return isDrawEnable_; }
		void SetDrawEnable(bool isEnable) override { isDrawEnable_ = isEnable; }
		bool LoadTextureByGuid(const Guid& g) override;
		void SetTextureGuid(const Guid& g) override;
		const Guid& GetTextureGuid() const override { return textureGuid_; }

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
		uint32_t GetDrawInstanceCount() const;
		const D3D12_GPU_DESCRIPTOR_HANDLE& GetTextureHandle() const { return textureHandle_; }
		const D3D12_GPU_DESCRIPTOR_HANDLE& GetNoiseMaskTextureHandle() const { return noiseMaskTextureHandle_; }
		BlendMode GetBlendMode() const { return blendMode_; }

		// setter
		void SetPosition(const CalyxEngine::Vector3& pos) override;

	private:
		void SyncEmitterDataFromBase();
		void DrawEmitterShapeInternal(bool effectPreview);

		CalyxEngine::Vector3 position_{0, 0, 0};
		bool	isInitialized = false;
		bool	hasInitializedOnce_ = false;
		bool	isPlaying_ = false;
		bool	isDrawEnable_ = true;
		Guid	textureGuid_{Guid::Empty()};
		D3D12_GPU_DESCRIPTOR_HANDLE textureHandle_{};
		D3D12_GPU_DESCRIPTOR_HANDLE noiseMaskTextureHandle_{};
		Guid noiseMaskTextureGuid_{Guid::Empty()};
		std::string noiseMaskTexturePath_;
		BlendMode blendMode_ = BlendMode::ADD;
		Vector2 noiseMaskScrollSpeed_{};

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
