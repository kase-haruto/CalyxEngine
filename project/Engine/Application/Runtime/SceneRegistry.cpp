#include <CalyxEngine/SceneRegistry.h>
#include <Engine/Scene/System/SceneManager.h>

namespace Calyx {
	SceneRegistry::SceneRegistry(CalyxEngine::SceneManager& sceneManager) : sceneManager_(sceneManager) {}
	bool SceneRegistry::OpenScene(const std::filesystem::path& path) { return sceneManager_.OpenScene(path); }
	bool SceneRegistry::OpenScene(const Guid& guid) { return sceneManager_.OpenScene(guid); }
	void SceneRegistry::RequestSceneChange(const std::filesystem::path& path) { sceneManager_.RequestSceneChange(path); }
	void SceneRegistry::RequestSceneChange(const Guid& guid) { sceneManager_.RequestSceneChange(guid); }
}
