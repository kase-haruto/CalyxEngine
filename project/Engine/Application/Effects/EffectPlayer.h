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

	struct EffectHandle {
		uint32_t id = 0;
		bool	 IsValid() const { return id != 0; }
	};

	/*-----------------------------------------------------------------------------------------
	 * EffectPlayer
	 * - EffectAssetからSceneObjectを作らずに実行時エフェクトを再生する
	 *---------------------------------------------------------------------------------------*/
	class EffectPlayer {
	public:
		CALYX_API void Initialize(FxSystem* fxSystem);
		CALYX_API void Update(float dt);
		CALYX_API void Clear();

		CALYX_API EffectHandle Play(const EffectAsset& asset,
						  const Vector3&	 position,
						  const Quaternion& rotation = Quaternion::MakeIdentity(),
						  const Vector3&	 scale	  = {1.0f, 1.0f, 1.0f});

		CALYX_API EffectHandle Play(const EffectAssetData& data,
						  const Vector3&		  position,
						  const Quaternion&	  rotation = Quaternion::MakeIdentity(),
						  const Vector3&		  scale	   = {1.0f, 1.0f, 1.0f});

		CALYX_API EffectHandle PlayFromName(const std::string& name,
								  const Vector3&			 position,
								  const Quaternion&			 rotation = Quaternion::MakeIdentity(),
								  const Vector3&			 scale	  = {1.0f, 1.0f, 1.0f});

		CALYX_API void Stop(EffectHandle handle);
		CALYX_API void SetTransform(EffectHandle handle,
						  const Vector3&	 position,
						  const Quaternion& rotation,
						  const Vector3&	 scale);

	private:
		struct RuntimeEmitter {
			std::shared_ptr<BaseEmitter> emitter;
			Guid						 ownerGuid;
			WorldTransformConfig		 localTransform;
		};

		struct EffectInstance {
			EffectHandle			 handle;
			Vector3					 position{0.0f, 0.0f, 0.0f};
			Quaternion				 rotation = Quaternion::MakeIdentity();
			Vector3					 scale{1.0f, 1.0f, 1.0f};
			std::vector<RuntimeEmitter> emitters;
			bool					 stopRequested = false;
		};

		void ApplyEmitterTransforms(EffectInstance& instance);
		bool IsFinished(const EffectInstance& instance) const;
		std::shared_ptr<BaseEmitter> CreateEmitter(const EffectEmitterAssetData& emitterData) const;

	private:
		FxSystem*				 fxSystem_ = nullptr;
		uint32_t				 nextHandleId_ = 1;
		std::vector<EffectInstance> instances_;
	};

} // namespace CalyxEngine
