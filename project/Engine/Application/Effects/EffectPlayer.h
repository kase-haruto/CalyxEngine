#pragma once

#include <Data/Engine/Configs/Scene/Objects/Particle/EffectConfig.h>
#include <Engine/Foundation/Export/CalyxAPI.h>
#include <Engine\Foundation\Math\Quaternion.h>
#include <Engine\Foundation\Math\Vector3.h>
#include <Engine\Foundation\Utility\Guid\Guid.h>

#include <filesystem>
#include <memory>
#include <vector>

namespace CalyxEngine {
	class BaseEmitter;
	class EffectAsset;
	class FxEmitter;
	class FxSystem;

	/*-----------------------------------------------------------------------------------------
	 * EffectHandle
	 * - Runtimeエフェクトインスタンスを識別する軽量ハンドル
	 * - 0を無効値として保持し、エフェクトの所有権は管理しない
	 *---------------------------------------------------------------------------------------*/
	struct EffectHandle {
		uint32_t id = 0; //< エフェクト識別子。0は無効値
		/** \brief 有効なエフェクトを参照しているか判定する \return 識別子が0以外の場合はtrue */
		bool	 IsValid() const { return id != 0; }
	};

	/*-----------------------------------------------------------------------------------------
	 * EffectPlayer
	 * - EffectAssetからSceneObjectを作らずに実行時エフェクトを再生する
	 *---------------------------------------------------------------------------------------*/
	class EffectPlayer {
	public:
		/** \brief Runtimeエミッターの登録先を設定する \param fxSystem 所有権を持たないエフェクト管理機能への参照 */
		CALYX_API void Initialize(FxSystem* fxSystem);
		/** \brief 再生中エフェクトを更新して終了済みインスタンスを解放する \param dt 前フレームからの経過時間（秒） */
		CALYX_API void Update(float dt);
		/** \brief 所有する全Runtimeエフェクトを登録先から解除する */
		CALYX_API void Clear();

		/** \brief アセットからRuntimeエフェクトを再生する \param asset 再生設定を保持するアセット \param position ワールド座標 \param rotation ワールド回転 \param scale ワールド拡大率 \return 再生を識別するハンドル。失敗時は無効値 */
		CALYX_API EffectHandle Play(const EffectAsset& asset,
						  const Vector3&	 position,
						  const Quaternion& rotation = Quaternion::MakeIdentity(),
						  const Vector3&	 scale	  = {1.0f, 1.0f, 1.0f});

		/** \brief シリアライズ済み設定からRuntimeエフェクトを再生する \param data エフェクト設定 \param position ワールド座標 \param rotation ワールド回転 \param scale ワールド拡大率 \return 再生を識別するハンドル。失敗時は無効値 */
		CALYX_API EffectHandle Play(const EffectAssetData& data,
						  const Vector3&		  position,
						  const Quaternion&	  rotation = Quaternion::MakeIdentity(),
						  const Vector3&		  scale	   = {1.0f, 1.0f, 1.0f});

		/** \brief 名前またはAssetパスからエフェクトを読み込んで再生する \param name エフェクト名またはAssetパス \param position ワールド座標 \param rotation ワールド回転 \param scale ワールド拡大率 \return 再生を識別するハンドル。失敗時は無効値 */
		CALYX_API EffectHandle PlayFromName(const std::string& name,
								  const Vector3&			 position,
								  const Quaternion&			 rotation = Quaternion::MakeIdentity(),
								  const Vector3&			 scale	  = {1.0f, 1.0f, 1.0f});

		/** \brief 指定エフェクトの停止と次回更新時の解放を要求する \param handle 停止対象のハンドル */
		CALYX_API void Stop(EffectHandle handle);
		/** \brief 再生中エフェクトのワールド変換を更新する \param handle 更新対象のハンドル \param position ワールド座標 \param rotation ワールド回転 \param scale ワールド拡大率 */
		CALYX_API void SetTransform(EffectHandle handle,
						  const Vector3&	 position,
						  const Quaternion& rotation,
						  const Vector3&	 scale);

	private:
		/*-----------------------------------------------------------------------------------------
		 * RuntimeEmitter
		 * - 1個のRuntimeエミッターとエフェクト内ローカル配置を束ねるデータ構造
		 * - FxSystemと共有するエミッターおよび登録解除用GUIDを保持
		 *---------------------------------------------------------------------------------------*/
		struct RuntimeEmitter {
			std::shared_ptr<BaseEmitter> emitter; //< FxSystemと共有所有するRuntimeエミッター
			Guid ownerGuid;                       //< FxSystemから登録解除するための所有者GUID
			WorldTransformConfig localTransform;  //< エフェクト原点からのローカル配置
		};

		/*-----------------------------------------------------------------------------------------
		 * EffectInstance
		 * - 1回のエフェクト再生に属するエミッターとワールド配置を保持するデータ構造
		 * - GPUリソースは所有せず、各エミッターとFxSystemへ管理を委譲
		 *---------------------------------------------------------------------------------------*/
		struct EffectInstance {
			EffectHandle handle;                    //< 外部操作に使用する再生ハンドル
			Vector3 position{0.0f, 0.0f, 0.0f};     //< エフェクト原点のワールド座標
			Quaternion rotation = Quaternion::MakeIdentity(); //< エフェクト原点のワールド回転
			Vector3 scale{1.0f, 1.0f, 1.0f};        //< エフェクト原点のワールド拡大率
			std::vector<RuntimeEmitter> emitters;   //< この再生が共有所有するRuntimeエミッター
			bool stopRequested = false;             //< 次回更新で登録解除する停止要求
		};

		/** \brief 親エフェクトとローカル配置から各エミッターのワールド変換を更新する \param instance 更新対象の再生インスタンス */
		void ApplyEmitterTransforms(EffectInstance& instance);
		/** \brief 全エミッターの再生と残存粒子が終了したか判定する \param instance 判定対象の再生インスタンス \return 安全に登録解除できる場合はtrue */
		bool IsFinished(const EffectInstance& instance) const;
		/** \brief 設定に応じたCPUまたはGPUエミッターを生成する \param emitterData エミッター設定 \return FxSystemと共有する新規エミッター */
		std::shared_ptr<BaseEmitter> CreateEmitter(const EffectEmitterAssetData& emitterData) const;

	private:
		FxSystem* fxSystem_ = nullptr;                  //< 所有権を持たないRuntimeエミッター登録先
		uint32_t nextHandleId_ = 1;                     //< 次に割り当てる識別子。0は使用しない
		std::vector<EffectInstance> instances_;         //< 現在再生中のエフェクトを所有する配列
	};

} // namespace CalyxEngine
