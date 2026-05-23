#pragma once
#include <Engine/Application/Effects/Particle/Emitter/FxEmitter.h>
#include <Engine/Application/Effects/Particle/Emitter/GpuFxEmitter.h>
#include <Engine/Renderer/Particle/ParticleRenderer.h>
#include <Engine/System/Event/EventBus.h>

#include <memory>
#include <vector>

struct Guid;
class SceneContext;

namespace CalyxEngine {
	/*-----------------------------------------------------------------------------------------
	 * FxSystem
	 * - エフェクト管理システムクラス
	 * - 各種エミッタ（CPU/GPU）のライフサイクル管理、シミュレーション、描画制御を担当
	 *---------------------------------------------------------------------------------------*/
	class FxSystem {
	public:
		//===================================================================*/
		//                   public methods
		//===================================================================*/
		/**
		 * \brief コンストラクタ
		 */
		explicit FxSystem(SceneContext* owner = nullptr);

		/**
		 * \brief デストラクタ
		 */
		~FxSystem();

		/**
		 * \brief エミッタを追加
		 * \param emitter 追加するエミッタ
		 * \param ownerGuid 所有オブジェクトのGUID
		 */
		void AddEmitter(const std::shared_ptr<BaseEmitter>& emitter,
						const Guid&										 ownerGuid);

		/**
		 * \brief SceneObjectに紐付かない実行時エミッタを追加
		 * \return 実行時所有者GUID。RemoveRuntimeEmitterOwnerに渡して削除する。
		 */
		Guid AddRuntimeEmitter(const std::shared_ptr<BaseEmitter>& emitter);

		/**
		 * \brief AddRuntimeEmitterで追加したエミッタ群を所有者GUID単位で削除
		 */
		void RemoveRuntimeEmitterOwner(const Guid& ownerGuid);

		/**
		 * \brief エミッタを削除
		 * \param emitter 削除するエミッタ（ポインタ検索）
		 */
		void RemoveEmitter(BaseEmitter* emitter);

		/**
		 * \brief エミッタの状態をGPUへ同期
		 */
		void SyncEmitters();

		/**
		 * \brief GPUエミッタをディスパッチ
		 * \param psoService パイプラインサービス
		 * \param cmdList コマンドリスト
		 */
		void DispatchEmitters(class PipelineService*	 psoService,
							  ID3D12GraphicsCommandList* cmdList);

		/**
		 * \brief 描画処理
		 * \param psoService パイプラインサービス
		 * \param cmdList コマンドリスト
		 */
		void Render(class PipelineService* psoService, ID3D12GraphicsCommandList* cmdList);

		/**
		 * \brief 全てのエミッタをクリア
		 */
		void Clear();

	private:
		//===================================================================*/
		//                    private methods
		//===================================================================*/
		/**
		 * \brief 指定したGUIDに紐付くエミッタを削除
		 * \param id 所有者のGUID
		 */
		void RemoveEmitterByGuid(const Guid& id);

	private:
		//===================================================================*/
		//                    private types
		//===================================================================*/
		struct CpuEmitterEntry {
			Guid ownerGuid; //< 所有者GUID
			std::weak_ptr<FxEmitter> emitter; //< CPUエミッタ
		};

		struct GpuEmitterEntry {
			Guid ownerGuid; //< 所有者GUID
			std::weak_ptr<GpuFxEmitter> emitter; //< GPUエミッタ
		};

		//===================================================================*/
		//                    private member variables
		//===================================================================*/
		std::vector<CpuEmitterEntry>	  cpuEmitters_;		 //< CPUエミッタリスト
		std::vector<GpuEmitterEntry>	  gpuEmitters_;		 //< GPUエミッタリスト
		std::unique_ptr<ParticleRenderer> particleRenderer_; //< パーティクルレンダラー
		SceneContext* owner_ = nullptr;

		EventBus::Connection connAdd_; //< 追加イベントコネクション
		EventBus::Connection connRem_; //< 削除イベントコネクション
	};
} // namespace CalyxEngine
