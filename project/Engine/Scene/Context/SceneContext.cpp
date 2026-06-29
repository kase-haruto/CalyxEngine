#include "SceneContext.h"

// engine
#include <Engine/Application/Effects/EffectPlayer.h>
#include <Engine/Application/Effects/FxSystem.h>
#include <Engine/Collision/CollisionManager.h>
#include <Engine/Graphics/Pipeline/Service/PipelineService.h>
#include <Engine/Physics/PhysicsSystem.h>
#include <Engine/Objects/3D/Actor/BaseGameObject.h>
#include <Engine/Renderer/Primitive/PrimitiveDrawer.h>
#include <Engine/Scene/SceneRuntime/IRuntimeBehaviour.h>
#include <functional>
#include <unordered_map>
#include <unordered_set>

SceneContext* SceneContext::current_ = nullptr;

SceneContext::~SceneContext() {
	if(current_ == this) {
		current_ = nullptr;
		// 破棄されるSceneSettingsへの非所有ポインタを残さず、Default fallbackへ戻す。
		CollisionLayerSettings::SetActiveSettings(nullptr);
	}
}

void SceneContext::MakeCurrent() {
	current_ = this;
	// ColliderとCollisionManagerが、このSceneContext固有のLayer一覧とMatrixを参照するよう切り替える。
	CollisionLayerSettings::SetActiveSettings(&settings_.GetCollisionSettings());
}

void SceneContext::Initialize(bool createDefaultLights) {
	MakeCurrent();

	objectLibrary_ = std::make_unique<SceneObjectLibrary>();
	objectLibrary_->SetOwner(this);
	lightLibrary_  = std::make_unique<LightLibrary>();
	fxSystem_	   = std::make_unique<CalyxEngine::FxSystem>(this);
	effectPlayer_  = std::make_unique<CalyxEngine::EffectPlayer>();
	effectPlayer_->Initialize(fxSystem_.get());

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
			if(ev.owner != this) return;
			SceneObject* raw = ev.sp.get();
			if(auto dir = std::dynamic_pointer_cast<DirectionalLight>(ev.sp)) {
				lightLibrary_->SetDirectionalLight(dir);
			} else if(auto point = std::dynamic_pointer_cast<PointLight>(ev.sp)) {
				lightLibrary_->AddPointLight(point);
			}
			// 登録されたリスナー全員に通知
			for(auto& cb : objectAddedCallbacks_) {
				if(cb) cb(raw);
			}
		});

	// --- ObjectRemoved を購読 ---
	connObjectRemoved_ = EventBus::Subscribe<ObjectRemoved>(
		[this](const ObjectRemoved& ev) {
			if(ev.owner != this) return;
			SceneObject* raw = ev.sp.get();
			if(auto dir = std::dynamic_pointer_cast<DirectionalLight>(ev.sp)) {
				if(lightLibrary_->GetDirectionalLight() == dir.get()) {
					lightLibrary_->SetDirectionalLight({});
				}
			} else if(auto point = std::dynamic_pointer_cast<PointLight>(ev.sp)) {
				lightLibrary_->RemovePointLight(point);
			}

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
	std::unordered_map<SceneObject*, std::shared_ptr<SceneObject>> objectByRaw;
	objectByRaw.reserve(objects.size());
	for(auto& sp : objects) {
		if(sp) objectByRaw[sp.get()] = sp;
	}

	std::unordered_map<SceneObject*, std::vector<SceneObject*>> updateChildren;
	updateChildren.reserve(objects.size());
	for(auto& sp : objects) {
		if(!sp) continue;
		if(auto parent = sp->GetParent()) {
			if(objectByRaw.contains(parent.get())) {
				updateChildren[parent.get()].push_back(sp.get());
			}
		}
	}

	for(auto& ownerSp : objects) {
		auto* owner = dynamic_cast<BaseGameObject*>(ownerSp.get());
		if(!owner) continue;
		for(auto& targetSp : objects) {
			if(!targetSp || targetSp.get() == owner) continue;
			if(owner->HasBoneParentTarget(&targetSp->GetWorldTransform())) {
				updateChildren[owner].push_back(targetSp.get());
			}
		}
	}

	std::vector<std::shared_ptr<SceneObject>> orderedObjects;
	orderedObjects.reserve(objects.size());
	std::unordered_set<SceneObject*> visiting;
	std::unordered_set<SceneObject*> visited;
	std::function<void(SceneObject*)> visit = [&](SceneObject* object) {
		if(!object || visited.contains(object) || visiting.contains(object)) return;
		visiting.insert(object);
		if(auto it = objectByRaw.find(object); it != objectByRaw.end()) {
			orderedObjects.push_back(it->second);
		}
		for(SceneObject* child : updateChildren[object]) {
			visit(child);
		}
		visiting.erase(object);
		visited.insert(object);
	};
	for(auto& sp : objects) {
		if(!sp) continue;
		if(auto parent = sp->GetParent(); parent && objectByRaw.contains(parent.get())) {
			continue;
		}
		visit(sp.get());
	}
	for(auto& sp : objects) {
		if(sp) visit(sp.get());
	}

	for(auto& sp : orderedObjects) {
		if(!sp) continue;

		if(runtimePass) {
			sp->Update(dt);
		}
		sp->AlwaysUpdate(alwaysDt);
	}

	if(effectPlayer_) {
		effectPlayer_->Update(alwaysDt);
	}

	PhysicsSystem::GetInstance()->ResolveAll();

	for(auto& sp : objects) {
		if(auto* object = dynamic_cast<BaseGameObject*>(sp.get())) {
			object->DrawCollider();
		}
	}

	CollisionManager::GetInstance()->UpdateCollisionAllCollider();

	lightLibrary_->CyncGpu();
	fxSystem_->SyncEmitters();
}

void SceneContext::PostUpdate(PipelineService* psoService, ID3D12GraphicsCommandList* cmd) {
	if(fxSystem_) {
		fxSystem_->DispatchEmitters(psoService, cmd);
	}
}

void SceneContext::Clear() {
	debugSelectedObject_ = nullptr;
	debugSelectedObjects_.clear();

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

	if(effectPlayer_) {
		effectPlayer_->Clear();
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
