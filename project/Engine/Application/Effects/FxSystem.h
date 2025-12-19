#pragma once
/* ========================================================================
/*  include space
/* ===================================================================== */
// engine
#include <Engine/Application/Effects/Particle/Emitter/FxEmitter.h>
#include <Engine/Application/Effects/Particle/Emitter/GpuFxEmitter.h>
#include <Engine/Renderer/Particle/ParticleRenderer.h>
#include <Engine/System/Event/EventBus.h>

// c++
#include <memory>
#include <vector>

struct Guid;

namespace CalyxEffect {
	/* ========================================================================
/*  effect system
/* ===================================================================== */
	class FxSystem {
	public:
		//===================================================================*/
		//                  public func
		//===================================================================*/
		FxSystem();
		~FxSystem();

		/// <summary>
		/// emitter追加（ownerGuid はこのエミッタを持つ SceneObject の GUID）
		/// </summary>
		void AddEmitter(const std::shared_ptr<BaseEmitter>& emitter,
						const Guid&										 ownerGuid);

		/// <summary>
		/// emitter削除（ポインタ一致で削除）
		/// </summary>
		void RemoveEmitter(BaseEmitter* emitter);

		/// <summary>
		/// エミッタgpu同期
		/// </summary>
		void SyncEmitters();

		/// <summary>
		/// GPU エミッタのディスパッチ
		/// </summary>
		void DispatchEmitters(class PipelineService*	 psoService,
							  ID3D12GraphicsCommandList* cmdList);

		/// <summary>
		/// 描画
		/// </summary>
		void Render(class PipelineService*, ID3D12GraphicsCommandList*);

		/// <summary>
		/// クリア
		/// </summary>
		void Clear();

	private:
		//===================================================================*/
		//                  private helper
		//===================================================================*/
		/// <summary>
		/// owner Guid から削除
		/// </summary>
		void RemoveEmitterByGuid(const Guid& id);

	private:
		//===================================================================*/
		//                  private variable
		//===================================================================*/

		struct CpuEmitterEntry {
			Guid								  ownerGuid;
			std::weak_ptr<FxEmitter> emitter;
		};
		struct GpuEmitterEntry {
			Guid									 ownerGuid;
			std::weak_ptr<GpuFxEmitter> emitter;
		};

		std::vector<CpuEmitterEntry>	  cpuEmitters_;		 //< cpuエミッタ
		std::vector<GpuEmitterEntry>	  gpuEmitters_;		 //< gpuエミッタ
		std::unique_ptr<ParticleRenderer> particleRenderer_; //< レンダラ

		EventBus::Connection connAdd_; //< 追加イベント
		EventBus::Connection connRem_; //< 削除イベント
	};
} // namespace CalyxEffect