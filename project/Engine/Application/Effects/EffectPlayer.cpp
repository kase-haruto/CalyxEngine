#include "EffectPlayer.h"

#include <Engine/Application/Effects/EffectAsset.h>
#include <Engine/Application/Effects/FxSystem.h>
#include <Engine/Application/Effects/Particle/Emitter/FxEmitter.h>
#include <Engine/Application/Effects/Particle/Emitter/GpuFxEmitter.h>

#include <algorithm>

namespace CalyxEngine {

	void EffectPlayer::Initialize(FxSystem* fxSystem) {
		fxSystem_ = fxSystem;
	}

	void EffectPlayer::Update([[maybe_unused]] float dt) {
		for(auto& instance : instances_) {
			ApplyEmitterTransforms(instance);
			for(auto& runtimeEmitter : instance.emitters) {
				if(runtimeEmitter.emitter) {
					runtimeEmitter.emitter->Update(dt);
				}
			}
		}

		for(auto it = instances_.begin(); it != instances_.end();) {
			if(it->stopRequested || IsFinished(*it)) {
				if(fxSystem_) {
					for(const auto& runtimeEmitter : it->emitters) {
						fxSystem_->RemoveRuntimeEmitterOwner(runtimeEmitter.ownerGuid);
					}
				}
				it = instances_.erase(it);
			} else {
				++it;
			}
		}
	}

	void EffectPlayer::Clear() {
		if(fxSystem_) {
			for(const auto& instance : instances_) {
				for(const auto& runtimeEmitter : instance.emitters) {
					fxSystem_->RemoveRuntimeEmitterOwner(runtimeEmitter.ownerGuid);
				}
			}
		}
		instances_.clear();
	}

	EffectHandle EffectPlayer::Play(const EffectAsset& asset,
									const Vector3&	   position,
									const Quaternion& rotation,
									const Vector3&	   scale) {
		return Play(asset.GetData(), position, rotation, scale);
	}

	EffectHandle EffectPlayer::Play(const EffectAssetData& data,
									const Vector3&		  position,
									const Quaternion&	  rotation,
									const Vector3&		  scale) {
		if(!fxSystem_) return {};

		EffectInstance instance{};
		instance.handle.id = nextHandleId_++;
		if(nextHandleId_ == 0) nextHandleId_ = 1;
		instance.position = position;
		instance.rotation = rotation;
		instance.scale	  = scale;

		for(const auto& emitterData : data.emitters) {
			auto emitter = CreateEmitter(emitterData);
			if(!emitter) continue;

			RuntimeEmitter runtimeEmitter{};
			runtimeEmitter.emitter		  = emitter;
			runtimeEmitter.localTransform = emitterData.transform;
			runtimeEmitter.ownerGuid	  = fxSystem_->AddRuntimeEmitter(emitter);
			instance.emitters.push_back(std::move(runtimeEmitter));
		}

		if(instance.emitters.empty()) return {};

		ApplyEmitterTransforms(instance);
		for(const auto& runtimeEmitter : instance.emitters) {
			runtimeEmitter.emitter->Play();
		}

		const EffectHandle handle = instance.handle;
		instances_.push_back(std::move(instance));
		return handle;
	}

	EffectHandle EffectPlayer::PlayFromName(const std::string& name,
											const Vector3&			   position,
											const Quaternion&		   rotation,
											const Vector3&			   scale) {
		EffectAsset asset;
		//パスから名前を抜き出す
		if(!asset.Load(name)) return {};
		return Play(asset, position, rotation, scale);
	}

	void EffectPlayer::Stop(EffectHandle handle) {
		if(!handle.IsValid()) return;
		for(auto& instance : instances_) {
			if(instance.handle.id == handle.id) {
				for(const auto& runtimeEmitter : instance.emitters) {
					runtimeEmitter.emitter->Stop();
				}
				instance.stopRequested = true;
				return;
			}
		}
	}

	void EffectPlayer::SetTransform(EffectHandle handle,
									const Vector3&	 position,
									const Quaternion& rotation,
									const Vector3&	 scale) {
		if(!handle.IsValid()) return;
		for(auto& instance : instances_) {
			if(instance.handle.id == handle.id) {
				instance.position = position;
				instance.rotation = rotation;
				instance.scale	  = scale;
				ApplyEmitterTransforms(instance);
				return;
			}
		}
	}

	void EffectPlayer::ApplyEmitterTransforms(EffectInstance& instance) {
		for(auto& runtimeEmitter : instance.emitters) {
			const auto& local = runtimeEmitter.localTransform;
			const Vector3 localOffset{
				local.translation.x * instance.scale.x,
				local.translation.y * instance.scale.y,
				local.translation.z * instance.scale.z};
			const Vector3 worldOffset = Quaternion::RotateVector(localOffset, instance.rotation);
			const Vector3 worldPos	 = instance.position + worldOffset;

			runtimeEmitter.emitter->SetWorldRotation(Quaternion::Multiply(instance.rotation, local.rotation));
			runtimeEmitter.emitter->SetWorldScale({
				instance.scale.x * local.scale.x,
				instance.scale.y * local.scale.y,
				instance.scale.z * local.scale.z});

			if(auto cpu = std::dynamic_pointer_cast<FxEmitter>(runtimeEmitter.emitter)) {
				cpu->SetPosition(worldPos);
			} else if(auto gpu = std::dynamic_pointer_cast<GpuFxEmitter>(runtimeEmitter.emitter)) {
				gpu->SetPosition(worldPos);
			}
		}
	}

	bool EffectPlayer::IsFinished(const EffectInstance& instance) const {
		for(const auto& runtimeEmitter : instance.emitters) {
			if(auto cpu = std::dynamic_pointer_cast<FxEmitter>(runtimeEmitter.emitter)) {
				if(cpu->IsPlaying() || !cpu->GetUnits().empty() || !cpu->GetTrailEmitter().History().Empty()) {
					return false;
				}
				continue;
			}

			if(runtimeEmitter.emitter && runtimeEmitter.emitter->IsPlaying()) {
				return false;
			}
		}
		return true;
	}

	std::shared_ptr<BaseEmitter> EffectPlayer::CreateEmitter(const EffectEmitterAssetData& emitterData) const {
		if(emitterData.isGpu) {
			auto emitter = std::make_shared<GpuFxEmitter>();
			emitter->Initialize();
			emitter->ApplyConfigFrom(emitterData.emitter);
			emitter->SetDrawEnable(emitterData.isDrawEnable);
			return emitter;
		}

		auto emitter = std::make_shared<FxEmitter>();
		emitter->ApplyConfigFrom(emitterData.emitter);
		emitter->SetDrawEnable(emitterData.isDrawEnable);
		return emitter;
	}

} // namespace CalyxEngine
