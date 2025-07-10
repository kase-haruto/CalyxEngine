#pragma once
/* ========================================================================
/* include space
/* ===================================================================== */

// engine
#include <Engine/objects/3D/Actor/Library/SceneObjectLibrary.h>
#include <Engine/Lighting/LightLibrary.h>

// c++
#include <memory>

using ObjectRemovedCallback = std::function<void(SceneObject*)>;

// forward declaration
class FxSystem;

class SceneContext{
public:
	SceneContext();
	~SceneContext();

	void Initialize();
	void Update();
	void Clear();

	void RemoveEditorObject(const std::shared_ptr<SceneObject>& obj);

	SceneObjectLibrary* GetObjectLibrary() const{ return objectLibrary_.get(); }

	template<typename TObject>
	TObject* AddEditorObject(std::shared_ptr<TObject> object);

	std::vector<std::shared_ptr<SceneObject>> GetEditorObjects();

	void AddOnObjectRemovedListener(std::function<void(SceneObject*)> cb){
		objectRemovedCallbacks_.push_back(std::move(cb));
	}

	std::string GetSceneName() const{ return sceneName_; }
	LightLibrary* GetLightLibrary() const{ return lightLibrary_.get(); }
	void SetSceneName(const std::string& name){ sceneName_ = name; }
	void SetOnEditorObjectRemoved(const ObjectRemovedCallback& callback){
		onEditorObjectRemoved_ = callback;
	}

	FxSystem* GetFxSystem() const{ return fxSystem_.get(); }
	std::shared_ptr<SceneObject> FindSharedObject(SceneObject* raw);

private:
	ObjectRemovedCallback onEditorObjectRemoved_;
	std::unique_ptr<SceneObjectLibrary> objectLibrary_;
	std::unique_ptr<LightLibrary> lightLibrary_;
	std::unique_ptr<FxSystem> fxSystem_;

private:
	std::vector<std::weak_ptr<SceneObject>> editorObjects_;
	std::vector<std::function<void(SceneObject*)>> objectRemovedCallbacks_;

	std::string sceneName_ = "scene";
	// シーン名
};

// ----------------------------------------------
// Template Implementation
// ----------------------------------------------
template<typename TObject>
TObject* SceneContext::AddEditorObject(std::shared_ptr<TObject> object){
	static_assert(std::is_base_of_v<SceneObject, TObject>, "TObject must derive from SceneObject");
	assert(object && "object must be a SceneObject");

	TObject* rawPtr = object.get();

	objectLibrary_->AddObject(object);
	editorObjects_.push_back(object);

	return rawPtr;
}