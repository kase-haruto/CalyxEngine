#pragma once

#include <Engine/Foundation/Utility/Guid/Guid.h>

#include <memory>
#include <string>
#include <vector>

class SceneContext;
class SceneObject;

namespace CalyxEngine {

	class SceneManager;

	class PrefabEditSession {
	public:
		void Ensure();
		void Reset();

		SceneContext* Context() const { return context_.get(); }
		const std::string& Path() const { return path_; }
		bool IsDirty() const { return dirty_; }

		std::shared_ptr<SceneObject> New(const std::string& rootTypeName, SceneManager* sceneManager);
		std::shared_ptr<SceneObject> Open(const std::string& path, SceneManager* sceneManager);
		void Update(float dt);

		std::vector<SceneObject*> GetRoots() const;
		void MarkUtilityObjects();
		void NormalizeRoots();

		bool Save(SceneManager* sceneManager);
		bool SaveAs(const std::string& path, SceneManager* sceneManager);
		bool ApplyOverridesFromInstance(const std::shared_ptr<SceneObject>& object, SceneManager* sceneManager);

	private:
		void SyncInstancesInCurrentScene(const Guid& prefabAssetGuid,
										 const std::string& prefabPath,
										 SceneManager* sceneManager);

		std::unique_ptr<SceneContext> context_;
		std::string path_;
		bool dirty_ = false;
	};

} // namespace CalyxEngine
