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

		SceneContext* previous = SceneContext::Current();

		context_ = std::make_unique<SceneContext>();
		context_->Initialize(false);
		context_->SetSceneName("ParticleEffectPreview");

		EnsureDefaultObject();

		if(auto* debugCamera = context_->GetCameraMgr()->GetDebug()) {
			debugCamera->GetWorldTransform().translation = {0.0f, 4.0f, -10.0f};
			debugCamera->GetWorldTransform().Update();
		}

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

		fx_ = context_->Instantiate<CalyxEngine::FxObject>("ParticlePreview");
		fx_->Initialize();
		fx_->SetEnablePicking(true);
		fx_->SetEnableRaycast(true);
		playedEmitterRevisions_[fx_.get()] = fx_->GetEmitterRevision();
	}

	void ParticlePreviewSession::Update(float dt) {
		if(!context_) return;

		SceneContext* previous = SceneContext::Current();
		context_->MakeCurrent();

		UpdateEmitterPlayback();

		context_->Update(dt, dt, false);

		if(previous && previous != context_.get()) {
			previous->MakeCurrent();
		}
	}

	void ParticlePreviewSession::UpdateEmitterPlayback() {
		if(!context_ || !context_->GetObjectLibrary()) return;

		std::unordered_map<SceneObject*, uint64_t> liveRevisions;
		for(const auto& object : context_->GetObjectLibrary()->GetAllObjectsShared()) {
			auto fx = std::dynamic_pointer_cast<FxObject>(object);
			if(!fx) continue;

			const uint64_t currentRevision = fx->GetEmitterRevision();
			const auto	   it			   = playedEmitterRevisions_.find(fx.get());
			if(it == playedEmitterRevisions_.end() || it->second != currentRevision) {
				fx->PlayAll();
			}
			liveRevisions[fx.get()] = currentRevision;
		}

		playedEmitterRevisions_ = std::move(liveRevisions);
	}

} // namespace CalyxEngine
