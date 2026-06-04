#pragma once

#include <Engine/Scene/Base/BaseScene.h>
#include <Engine/Scene/System/SceneManager.h>
#include <Engine/Scene/Utility/SceneUtility.h>

#include <memory>

namespace Calyx {

	class SceneRegistry {
	public:
		explicit SceneRegistry(CalyxEngine::SceneManager& sceneManager);

		size_t AddScene(CalyxEngine::SceneId id, std::unique_ptr<BaseScene> scene);
		void SetStartupScene(CalyxEngine::SceneId id);

		template <class TScene, class... Args>
		size_t AddScene(CalyxEngine::SceneId id, Args&&... args) {
			return AddScene(id, std::make_unique<TScene>(std::forward<Args>(args)...));
		}

	private:
		CalyxEngine::SceneManager& sceneManager_;
	};

} // namespace Calyx
