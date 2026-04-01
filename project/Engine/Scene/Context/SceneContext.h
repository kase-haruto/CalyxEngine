#pragma once
/* ========================================================================
/* include space
/* ===================================================================== */

// engine
#include <Engine/Application/Effects/FxSystem.h>
#include <Engine/Graphics/Camera/Manager/CameraManager.h>
#include <Engine/Lighting/LightLibrary.h>
#include <Engine/System/Event/EventBus.h>
#include <Engine/objects/3D/Actor/Library/SceneObjectLibrary.h>


// c++
#include <functional>
#include <memory>
#include <string>
#include <vector>

using ObjectRemovedCallback = std::function<void(SceneObject*)>;
using ObjectAddedCallback	= std::function<void(SceneObject*)>;

/*-----------------------------------------------------------------------------------------
 * SceneContext
 * - シーンコンテキストクラス
 * - シーン内のオブジェクト・ライト・カメラ・エフェクトシステムを統合管理
 *---------------------------------------------------------------------------------------*/
class SceneContext {
public:
	SceneContext()	= default;
	~SceneContext() = default;

	void Initialize(bool createDefaultLights = true);
	void Update(float dt, float alwaysDt, bool runtimePass);
	/**
	 * \brief 更新後処理
	 * \param psoService
	 * \param cmd
	 */
	void PostUpdate(class PipelineService*, ID3D12GraphicsCommandList*);
	void Clear();

	/* ---------- object API ---------- */

	/**
	 * インスタンスの作成とライブラリに登録
	 * @tparam TObject
	 * @tparam Args
	 * @param args
	 * @return
	 */
	template <class TObject, class... Args>
	std::shared_ptr<TObject> Instantiate(Args&&... args);

	/**
	 * 最初に見つかった型Tのオブジェクトを返す
	 */
	template <typename T>
	std::shared_ptr<T> FindFirst() const;

	/**
	 * 名前からオブジェクトを探す
	 * @tparam T
	 * @param name
	 * @return 検索結果のオブジェクト（見つからなければ nullptr）
	 */
	template <typename T>
	std::shared_ptr<T> FindObjectByName(const std::string& name) const;

	/* ---------- accessors ----------- */
	// getter
	SceneObjectLibrary*	   GetObjectLibrary() const { return objectLibrary_.get(); }
	LightLibrary*		   GetLightLibrary() const { return lightLibrary_.get(); }
	CalyxEffect::FxSystem* GetFxSystem() const { return fxSystem_.get(); }
	std::string			   GetSceneName() const { return sceneName_; }
	bool				   IsRuntime() const { return isRuntime_; }
	CameraManager*		   GetCameraMgr() { return cameraMgr_.get(); }
	SceneObject*		   GetDebugSelectedObject() const { return debugSelectedObject_; }

	// setter
	void SetSceneName(const std::string& n) { sceneName_ = n; }
	void SetRuntime(bool f) { isRuntime_ = f; }
	void SetDebugSelectedObject(SceneObject* obj) { debugSelectedObject_ = obj; }

	/* ---------- callbacks ----------- */
	/// 個別削除時に飛ぶコールバック
	void AddOnObjectRemovedListener(ObjectRemovedCallback cb) {
		objectRemovedCallbacks_.push_back(std::move(cb));
	}

	/// Clear 時など、Editor 側で一括ハンドリングしたい
	void SetOnEditorObjectRemoved(ObjectRemovedCallback cb) {
		onEditorObjectRemoved_ = std::move(cb);
	}

	void AddOnObjectAddedListener(ObjectAddedCallback cb) {
		objectAddedCallbacks_.push_back(std::move(cb));
	}

	/* ---------- utils --------------- */
	std::shared_ptr<SceneObject> FindSharedObject(SceneObject* raw);
	void						 AddObject(const std::shared_ptr<SceneObject>& obj);
	void						 RemoveObject(const std::shared_ptr<SceneObject>& obj);

	/* ---------- Current ------------- */
	static SceneContext* Current() { return current_; }
	void				 MakeCurrent() { current_ = this; }

private:
	std::unique_ptr<SceneObjectLibrary>	   objectLibrary_;
	std::unique_ptr<LightLibrary>		   lightLibrary_;
	std::unique_ptr<CalyxEffect::FxSystem> fxSystem_;
	std::unique_ptr<CameraManager>		   cameraMgr_;

	ObjectRemovedCallback			   onEditorObjectRemoved_;
	std::vector<ObjectRemovedCallback> objectRemovedCallbacks_;
	std::vector<ObjectAddedCallback>   objectAddedCallbacks_;

	std::string sceneName_ = "scene";
	bool		isRuntime_ = false;

	SceneObject* debugSelectedObject_ = nullptr;

	EventBus::Connection connObjectAdded_;
	EventBus::Connection connObjectRemoved_;

	static SceneContext* current_;
};

// --------------------------- template implementations ------------------------

template <class TObject, class... Args>
std::shared_ptr<TObject> SceneContext::Instantiate(Args&&... args) {
	static_assert(std::is_base_of_v<SceneObject, TObject>,
				  "TObject must derive from SceneObject");

	auto obj = std::make_shared<TObject>(std::forward<Args>(args)...);
	objectLibrary_->AddObject(obj);
	return obj;
}

template <typename T>
std::shared_ptr<T> SceneContext::FindFirst() const {
	for(const auto& obj : objectLibrary_->GetAllObjectsShared()) {
		if(auto casted = std::dynamic_pointer_cast<T>(obj)) {
			return casted;
		}
	}
	return nullptr;
}

template <typename T>
std::shared_ptr<T> SceneContext::FindObjectByName(const std::string& name) const {
	for(const auto& obj : objectLibrary_->GetAllObjectsShared()) {
		if(obj && obj->GetName() == name) {
			if constexpr(std::is_same_v<T, SceneObject>) {
				return obj;
			} else {
				if(auto casted = std::dynamic_pointer_cast<T>(obj)) {
					return casted;
				}
			}
		}
	}
	return nullptr;
}