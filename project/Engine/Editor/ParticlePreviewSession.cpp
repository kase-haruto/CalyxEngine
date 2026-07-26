#include "ParticlePreviewSession.h"

#include <Engine/Application/Effects/FxObject.h>
#include <Engine/Graphics/Camera/3d/DebugCamera.h>
#include <Engine/Graphics/Camera/Manager/CameraManager.h>
#include <Engine/Objects/3D/Actor/Library/SceneObjectLibrary.h>
#include <Engine/Objects/Transform/Transform.h>
#include <Engine/Scene/Context/SceneContext.h>

namespace CalyxEngine {

	void ParticlePreviewSession::Ensure() {
		if(context_) return;

		// Preview Context初期化によるCurrent切替に備え、元のEditor Sceneを退避する。
		SceneContext* previous = SceneContext::Current();

		// Runtime SceneへPreview用Emitterを混入させないよう、独立したSceneContextを所有する。
		context_ = std::make_unique<SceneContext>();
		context_->Initialize(false);
		context_->SetSceneName("ParticleEffectPreview");

		// 空Previewでも即座に編集できる既定FxObjectを一つ生成する。
		EnsureDefaultObject();

		if(auto* debugCamera = context_->GetCameraMgr()->GetDebug()) {
			debugCamera->GetWorldTransform().translation = {0.0f, 4.0f, -10.0f};
			debugCamera->GetWorldTransform().Update();
		}

		// 初期化後は呼び出し元Contextを復元し、他Panelの参照先を変更しない。
		if(previous) {
			previous->MakeCurrent();
		}
	}

	std::shared_ptr<FxObject> ParticlePreviewSession::Object() const {
		if(fx_) return fx_;
		if(!context_ || !context_->GetObjectLibrary()) return nullptr;

		for(const auto& object : context_->GetObjectLibrary()->GetAllObjectsShared()) {
			if(auto fx = std::dynamic_pointer_cast<FxObject>(object)) {
				return fx;
			}
		}
		return nullptr;
	}

	void ParticlePreviewSession::EnsureDefaultObject() {
		if(!context_ || fx_) return;

		// FxObjectはPreview Contextが所有し、Sessionは選択・更新用の共有参照を保持する。
		fx_ = context_->Instantiate<CalyxEngine::FxObject>("ParticlePreview");
		fx_->Initialize();
		fx_->SetEnablePicking(true);
		fx_->SetEnableRaycast(true);
		// 初期Revisionを記録し、未変更Emitterを毎フレーム再生し直さない。
		playedEmitterRevisions_[fx_.get()] = fx_->GetEmitterRevision();
	}

	void ParticlePreviewSession::Update(float dt) {
		if(!context_) return;

		// Emitter更新中だけPreview ContextをCurrentにし、Resource/Manager参照をPreview側へ向ける。
		SceneContext* previous = SceneContext::Current();
		context_->MakeCurrent();

		UpdateEmitterPlayback();

		context_->Update(dt, dt, false);

		// Update完了後にEditor Sceneへ戻し、RuntimeやHierarchy更新へ影響させない。
		if(previous && previous != context_.get()) {
			previous->MakeCurrent();
		}
	}

	void ParticlePreviewSession::UpdateEmitterPlayback() {
		if(!context_ || !context_->GetObjectLibrary()) return;

		// 現在存在するFxだけでMapを再構築し、削除済みObjectのraw pointer Keyを残さない。
		std::unordered_map<SceneObject*, uint64_t> liveRevisions;
		for(const auto& object : context_->GetObjectLibrary()->GetAllObjectsShared()) {
			auto fx = std::dynamic_pointer_cast<FxObject>(object);
			if(!fx) continue;

			const uint64_t currentRevision = fx->GetEmitterRevision();
			const auto	   it			   = playedEmitterRevisions_.find(fx.get());
			// Emitter構成が変更されたときだけ再生し、Editorでの連続Previewを維持する。
			if(it == playedEmitterRevisions_.end() || it->second != currentRevision) {
				fx->PlayAll();
			}
			liveRevisions[fx.get()] = currentRevision;
		}

		playedEmitterRevisions_ = std::move(liveRevisions);
	}

} // namespace CalyxEngine
