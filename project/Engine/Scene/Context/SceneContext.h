#pragma once
/* ========================================================================
/* include space
/* ===================================================================== */

// engine
#include <Engine/objects/3D/Actor/Library/SceneObjectLibrary.h>
#include <Engine/Lighting/LightLibrary.h>
#include <Engine/Application/Effects/FxSystem.h>
#include <Engine/Graphics/Camera/Manager/CameraManager.h>

// c++
#include <memory>

using ObjectRemovedCallback = std::function<void(SceneObject*)>;

class SceneContext {
public:
	SceneContext() = default;
	~SceneContext() = default;

	void Initialize(bool createDefaultLights = true);
	void Update(float dt, bool runtimePass);
	void RunRuntimeBootstrap();
	void PostUpdate(class PipelineService*, ID3D12GraphicsCommandList*);
	void Clear();

	/* ---------- object API ---------- */
	template<class TObject, class... Args>
	std::shared_ptr<TObject> Instantiate(Args&&... args);

	template<typename T>
	std::shared_ptr<T> FindFirst() const;

	void RemoveEditorObject(const std::shared_ptr<SceneObject>& object);

	/* ---------- accessors ----------- */
	SceneObjectLibrary* GetObjectLibrary() const { return objectLibrary_.get(); }
	LightLibrary* GetLightLibrary()  const { return lightLibrary_.get(); }
	FxSystem* GetFxSystem()      const { return fxSystem_.get(); }

	std::string GetSceneName() const { return sceneName_; }
	void        SetSceneName(const std::string& n) { sceneName_ = n; }

	bool IsRuntime() const { return isRuntime_; }
	void SetRuntime(bool f) { isRuntime_ = f; }
	CameraManager* GetCameraMgr() { return cameraMgr_.get(); }

	/* ---------- callbacks ----------- */
	void AddOnObjectRemovedListener(ObjectRemovedCallback cb) { objectRemovedCallbacks_.push_back(std::move(cb)); }
	void SetOnEditorObjectRemoved(ObjectRemovedCallback cb) { onEditorObjectRemoved_ = std::move(cb); }

	/* ---------- utils --------------- */
	std::shared_ptr<SceneObject> FindSharedObject(SceneObject* raw);

	void AddObject(const std::shared_ptr<SceneObject>& obj);

	void RemoveObject(const std::shared_ptr<SceneObject>& obj);

	/* ---------- Current ------------- */
	static SceneContext* Current() { return current_; }
	void MakeCurrent() { current_ = this; }

private:
	std::unique_ptr<SceneObjectLibrary> objectLibrary_;
	std::unique_ptr<LightLibrary>       lightLibrary_;
	std::unique_ptr<FxSystem>           fxSystem_;
	std::unique_ptr<CameraManager>      cameraMgr_;

	ObjectRemovedCallback               onEditorObjectRemoved_;
	std::vector<ObjectRemovedCallback>  objectRemovedCallbacks_;

	std::string sceneName_ = "scene";
	bool        isRuntime_ = false;

	static SceneContext* current_;
};


// --------------------------- template implementations ------------------------
template<class TObject, class... Args>
std::shared_ptr<TObject> SceneContext::Instantiate(Args&&... args) {
	static_assert(std::is_base_of_v<SceneObject, TObject>,
				  "TObject must derive from SceneObject");
	auto obj = std::make_shared<TObject>(std::forward<Args>(args)...);
	objectLibrary_->AddObject(obj);
	return obj;
}

template<typename T>
std::shared_ptr<T> SceneContext::FindFirst() const {
	for (const auto& obj : objectLibrary_->GetAllObjectsShared()) {
		if (auto casted = std::dynamic_pointer_cast<T>(obj)) {
			return casted;
		}
	}
	return nullptr;
}