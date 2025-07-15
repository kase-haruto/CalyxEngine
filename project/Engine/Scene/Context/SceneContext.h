#pragma once
/* ========================================================================
/* include space
/* ===================================================================== */

// engine
#include <Engine/objects/3D/Actor/Library/SceneObjectLibrary.h>
#include <Engine/Lighting/LightLibrary.h>
#include <Engine/Application/Effects/FxSystem.h>
#include <Engine/Extensions/SkyBox/SkyBox.h>

#include <Engine/Renderer/Sprite/SpriteRenderer.h>
#include <Engine/Renderer/Model/ModelRenderer.h>
#include <Engine/Graphics/RenderTarget/Detail/RenderTargetDetail.h>

// c++
#include <memory>
#include <d3d12.h>

class SceneObject;

using ObjectRemovedCallback = std::function<void(SceneObject*)>;

class SceneContext{
public:
	SceneContext() = default;
	~SceneContext() = default;

	void Initialize();
	void Update();
	void Render(ID3D12GraphicsCommandList* cmd,
				class PipelineService* pso,
				RenderTargetType           rtType);
	void Clear();

	// object API ------------------------------------------------------------
	template<class TObject>
	TObject* AddEditorObject(std::shared_ptr<TObject> object);
	void RemoveEditorObject(const std::shared_ptr<SceneObject>& object);

	// accessors --------------------------------------------------------------
	SceneObjectLibrary* GetObjectLibrary() const{ return objectLibrary_.get(); }
	LightLibrary* GetLightLibrary()  const{ return lightLibrary_.get(); }
	FxSystem* GetFxSystem()      const{ return fxSystem_.get(); }

	std::string GetSceneName() const{ return sceneName_; }
	void SetSceneName(const std::string& n){ sceneName_ = n; }

	// callbacks --------------------------------------------------------------
	void AddOnObjectRemovedListener(ObjectRemovedCallback cb){ objectRemovedCallbacks_.push_back(std::move(cb)); }
	void SetOnEditorObjectRemoved(ObjectRemovedCallback cb){ onEditorObjectRemoved_ = std::move(cb); }

	// utils ------------------------------------------------------------------
	std::shared_ptr<SceneObject> FindSharedObject(SceneObject* raw);

private:
	// subsystems -------------------------------------------------------------
	std::unique_ptr<SceneObjectLibrary> objectLibrary_;
	std::unique_ptr<LightLibrary> lightLibrary_;
	std::unique_ptr<FxSystem> fxSystem_;

	// render -----------------------------------------------------------------
	std::unique_ptr<SkyBox>         skyBox_;
	std::unique_ptr<SpriteRenderer> spriteRenderer_;
	std::unique_ptr<ModelRenderer>  modelRenderer_;

	// event helpers ----------------------------------------------------------
	ObjectRemovedCallback onEditorObjectRemoved_;
	std::vector<ObjectRemovedCallback> objectRemovedCallbacks_;

	std::string sceneName_ = "scene";
};

template<class TObject>
TObject* SceneContext::AddEditorObject(std::shared_ptr<TObject> object){
	static_assert(std::is_base_of_v<SceneObject, TObject>, "TObject must derive from SceneObject");
	assert(object && "object must be a SceneObject");

	TObject* raw = object.get();
	objectLibrary_->AddObject(std::move(object));
	return raw;
}