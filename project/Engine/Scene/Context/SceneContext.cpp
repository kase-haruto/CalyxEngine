#include "SceneContext.h"

// engine
#include <Engine/Application/Effects/FxSystem.h>
#include <Engine/Collision/CollisionManager.h>
#include <Engine/Graphics/Pipeline/Service/PipelineService.h>
#include <Engine/Renderer/Primitive/PrimitiveDrawer.h>
#include <Engine/Scene/Runtime/IRuntimeBehaviour.h>

SceneContext* SceneContext::current_ = nullptr;

void SceneContext::Initialize(bool createDefaultLights) {
	MakeCurrent();

	objectLibrary_ = std::make_unique<SceneObjectLibrary>();
	lightLibrary_  = std::make_unique<LightLibrary>();
	fxSystem_	   = std::make_unique<CalyxEffect::FxSystem>();

	if(createDefaultLights) {
		auto dir = Instantiate<DirectionalLight>("DirectionalLight");
		dir->SetEnableRaycast(false);

		auto pt = Instantiate<PointLight>("PointLight");
		pt->SetEnableRaycast(false);

		lightLibrary_->SetDirectionalLight(dir);
		lightLibrary_->SetPointLight(pt);
	}

	cameraMgr_ = std::make_unique<CameraManager>();
	cameraMgr_->Initialize(this);

	// --- ObjectAdded を購読 ---
	connObjectAdded_ = EventBus::Subscribe<ObjectAdded>(
		[this](const ObjectAdded& ev) {
			SceneObject* raw = ev.sp.get();
			// 登録されたリスナー全員に通知
			for(auto& cb : objectAddedCallbacks_) {
				if(cb) cb(raw);
			}
		});

	// --- ObjectRemoved を購読 ---
	connObjectRemoved_ = EventBus::Subscribe<ObjectRemoved>(
		[this](const ObjectRemoved& ev) {
			SceneObject* raw = ev.sp.get();

			// Editor 用（1個だけ）
			if(onEditorObjectRemoved_) {
				onEditorObjectRemoved_(raw);
			}
			// 通常リスナー（複数）
			for(auto& cb : objectRemovedCallbacks_) {
				if(cb) cb(raw);
			}
		});
}

void SceneContext::Update(float dt, float alwaysDt, bool runtimePass) {
	if(!objectLibrary_) return;

	// 毎フレーム一度だけロックして使い回す
	auto objects = objectLibrary_->GetAllObjectsShared();

	// Runtime パス（ゲーム実行中のみ）
	if(runtimePass) {
		for(auto& sp : objects) {
			if(sp) sp->Update(dt);
		}
	}

	// Always パス（エディタ / ランタイム共通）
	for(auto& sp : objects) {
		if(sp) sp->AlwaysUpdate(alwaysDt);
	}

	lightLibrary_->CyncGpu();
	fxSystem_->SyncEmitters();
}

void SceneContext::PostUpdate(PipelineService* psoService, ID3D12GraphicsCommandList* cmd) {
	if(fxSystem_) {
		fxSystem_->DispatchEmitters(psoService, cmd);
	}
}

void SceneContext::Clear() {
	// Editor 側への通知（エディタで持っているハンドルを掃除させる）
	if(objectLibrary_) {
		if(onEditorObjectRemoved_) {
			for(auto& sp : objectLibrary_->GetAllObjectsShared()) {
				if(!sp) continue;
				onEditorObjectRemoved_(sp.get());
			}
		}
		// Destroy → EventBus(ObjectRemoved) は SceneObjectLibrary::Clear が行う
		objectLibrary_->Clear();
	}

	if(fxSystem_) {
		fxSystem_->Clear();
	}

	CollisionManager::GetInstance()->ClearColliders();
	PrimitiveDrawer::GetInstance()->ClearMesh();
}

std::shared_ptr<SceneObject> SceneContext::FindSharedObject(SceneObject* raw) {
	if(!objectLibrary_ || !raw) return nullptr;

	for(auto& sp : objectLibrary_->GetAllObjectsShared()) {
		if(sp.get() == raw) return sp;
	}
	return nullptr;
}

void SceneContext::AddObject(const std::shared_ptr<SceneObject>& obj) {
	if(!objectLibrary_ || !obj) return;
	objectLibrary_->AddObject(obj);
}

void SceneContext::RemoveObject(const std::shared_ptr<SceneObject>& obj) {
	if(!objectLibrary_ || !obj) return;

	// ランタイム／内部からの削除要求
	objectLibrary_->RemoveObject(obj);

	// 共通の削除リスナにも通知しておく
	for(auto& cb : objectRemovedCallbacks_) {
		if(cb) cb(obj.get());
	}
}
