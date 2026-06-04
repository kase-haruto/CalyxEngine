#include <CalyxEngine/SceneRegistry.h>

namespace Calyx {

	SceneRegistry::SceneRegistry(CalyxEngine::SceneManager& sceneManager)
		: sceneManager_(sceneManager) {}

	size_t SceneRegistry::AddScene(CalyxEngine::SceneId id, std::unique_ptr<BaseScene> scene) {
		return sceneManager_.AddScene(id, std::move(scene));
	}

	void SceneRegistry::SetStartupScene(CalyxEngine::SceneId id) {
		sceneManager_.SetCurrent(id);
	}

} // namespace Calyx
