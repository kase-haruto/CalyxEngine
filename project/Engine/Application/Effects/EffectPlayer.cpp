#include "EffectPlayer.h"

#include <Engine/Application/Effects/EffectAsset.h>
#include <Engine/Application/Effects/FxSystem.h>
#include <Engine/Application/Effects/Particle/Emitter/FxEmitter.h>
#include <Engine/Application/Effects/Particle/Emitter/GpuFxEmitter.h>

#include <algorithm>

namespace CalyxEngine {

	void EffectPlayer::Initialize(FxSystem* fxSystem) {
		// エミッターの実体はFxSystemと共有するため、登録先だけを非所有参照として保持する。
		fxSystem_ = fxSystem;
	}

	void EffectPlayer::Update([[maybe_unused]] float dt) {
		// 親エフェクトの移動を反映してから各Runtimeエミッターを進行させる。
		for(auto& instance : instances_) {
			ApplyEmitterTransforms(instance);

			// 無効な共有参照を許容し、部分的な生成失敗が他エミッターの更新を止めないようにする。
			for(auto& runtimeEmitter : instance.emitters) {
				if(runtimeEmitter.emitter) {
					runtimeEmitter.emitter->Update(dt);
				}
			}
		}

		// 停止要求または全粒子の消滅を確認してからFxSystemの登録を解除する。
		for(auto it = instances_.begin(); it != instances_.end();) {
			if(it->stopRequested || IsFinished(*it)) {
				// FxSystem側の描画・更新一覧から先に外し、破棄後の共有参照を残さない。
				if(fxSystem_) {
					for(const auto& runtimeEmitter : it->emitters) {
						fxSystem_->RemoveRuntimeEmitterOwner(runtimeEmitter.ownerGuid);
					}
				}

				// 登録解除後にPlayer側の共有所有権を解放する。
				it = instances_.erase(it);
			} else {
				++it;
			}
		}
	}

	void EffectPlayer::Clear() {
		// Editorプレビュー終了時にも残存エミッターが描画リストへ残らないよう先に登録解除する。
		if(fxSystem_) {
			for(const auto& instance : instances_) {
				for(const auto& runtimeEmitter : instance.emitters) {
					fxSystem_->RemoveRuntimeEmitterOwner(runtimeEmitter.ownerGuid);
				}
			}
		}

		// FxSystemとの関連を全て切った後で再生インスタンスの共有所有権を解放する。
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

		// ハンドル0を無効値として予約したまま、再生単位の所有コンテナを構築する。
		EffectInstance instance{};
		instance.handle.id = nextHandleId_++;

		// uint32_tの周回時にも0を外部へ返さないよう次回IDを1へ戻す。
		if(nextHandleId_ == 0) nextHandleId_ = 1;
		instance.position = position;
		instance.rotation = rotation;
		instance.scale	  = scale;

		// シリアライズ設定からCPU/GPUエミッターを生成し、FxSystemへRuntime登録する。
		for(const auto& emitterData : data.emitters) {
			auto emitter = CreateEmitter(emitterData);
			// 未対応または生成失敗したEmitterだけを除外し、他の設定は再生を継続する。
			if(!emitter) continue;

			RuntimeEmitter runtimeEmitter{};
			runtimeEmitter.emitter		  = emitter;
			runtimeEmitter.localTransform = emitterData.transform;

			// FxSystemへ登録して得たGUIDを、停止・終了時の解除キーとして保持する。
			runtimeEmitter.ownerGuid	  = fxSystem_->AddRuntimeEmitter(emitter);
			instance.emitters.push_back(std::move(runtimeEmitter));
		}

		// 有効なEmitterが1個もなければインスタンスを登録せず無効ハンドルを返す。
		if(instance.emitters.empty()) return {};

		// 最初の描画フレームから正しいワールド配置になるよう、再生開始前に変換を適用する。
		ApplyEmitterTransforms(instance);
		for(const auto& runtimeEmitter : instance.emitters) {
			// 全Emitterの配置が確定した後に一斉に再生を開始する。
			runtimeEmitter.emitter->Play();
		}

		// move後にinstanceへアクセスしないよう、外部へ返すハンドルを先に退避する。
		const EffectHandle handle = instance.handle;
		instances_.push_back(std::move(instance));
		return handle;
	}

	EffectHandle EffectPlayer::PlayFromName(const std::string& name,
											const Vector3&			   position,
											const Quaternion&		   rotation,
											const Vector3&			   scale) {
		EffectAsset asset;
		// AssetDatabase互換の名前解決はEffectAssetへ委譲し、読込失敗時は無効ハンドルを返す。
		if(!asset.Load(name)) return {};
		return Play(asset, position, rotation, scale);
	}

	void EffectPlayer::Stop(EffectHandle handle) {
		// 無効ハンドルでは再生一覧を走査しない。
		if(!handle.IsValid()) return;
		for(auto& instance : instances_) {
			if(instance.handle.id == handle.id) {
				// 新規発生を止めた後、Updateで残存登録をまとめて解除する。
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
		// 無効ハンドルでは再生一覧を走査しない。
		if(!handle.IsValid()) return;
		for(auto& instance : instances_) {
			if(instance.handle.id == handle.id) {
				instance.position = position;
				instance.rotation = rotation;
				instance.scale	  = scale;

				// 次のUpdateを待たず、Editor操作と同じフレームで表示位置を同期する。
				ApplyEmitterTransforms(instance);
				return;
			}
		}
	}

	void EffectPlayer::ApplyEmitterTransforms(EffectInstance& instance) {
		// 親の非一様スケールと回転をローカルオフセットへ適用してワールド配置を復元する。
		for(auto& runtimeEmitter : instance.emitters) {
			const auto& local = runtimeEmitter.localTransform;

			// 親の非一様スケールをローカル移動量へ成分ごとに適用する。
			const Vector3 localOffset{
				local.translation.x * instance.scale.x,
				local.translation.y * instance.scale.y,
				local.translation.z * instance.scale.z};
			// スケール済みオフセットを親回転させてワールド空間へ変換する。
			const Vector3 worldOffset = Quaternion::RotateVector(localOffset, instance.rotation);
			const Vector3 worldPos	 = instance.position + worldOffset;

			// ローカル回転を親回転へ合成し、Emitter共通のワールド回転として設定する。
			runtimeEmitter.emitter->SetWorldRotation(Quaternion::Multiply(instance.rotation, local.rotation));

			// 親子の拡大率を成分ごとに合成し、非一様スケールを維持する。
			runtimeEmitter.emitter->SetWorldScale({
				instance.scale.x * local.scale.x,
				instance.scale.y * local.scale.y,
				instance.scale.z * local.scale.z});

			// CPU/GPUで位置設定APIが異なるため、実体型に応じた境界でのみ分岐する。
			if(auto cpu = std::dynamic_pointer_cast<FxEmitter>(runtimeEmitter.emitter)) {
				cpu->SetPosition(worldPos);
			} else if(auto gpu = std::dynamic_pointer_cast<GpuFxEmitter>(runtimeEmitter.emitter)) {
				gpu->SetPosition(worldPos);
			}
		}
	}

	bool EffectPlayer::IsFinished(const EffectInstance& instance) const {
		// CPUエミッターは再生フラグだけでなく、残存粒子とTrail履歴の消滅まで待つ。
		for(const auto& runtimeEmitter : instance.emitters) {
			if(auto cpu = std::dynamic_pointer_cast<FxEmitter>(runtimeEmitter.emitter)) {
				// 発生停止後もParticleまたはTrail履歴が残る間は描画登録を維持する。
				if(cpu->IsPlaying() || !cpu->GetUnits().empty() || !cpu->GetTrailEmitter().History().Empty()) {
					return false;
				}
				continue;
			}

			// GPU Emitterは再生フラグを終了判定の契約として使用する。
			if(runtimeEmitter.emitter && runtimeEmitter.emitter->IsPlaying()) {
				return false;
			}
		}
		return true;
	}

	std::shared_ptr<BaseEmitter> EffectPlayer::CreateEmitter(const EffectEmitterAssetData& emitterData) const {
		// GPUリソースの初期化と所有はエミッター自身へ委譲し、Playerは共有参照だけを保持する。
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
