#pragma once

#include <Engine/Foundation/Export/CalyxAPI.h>

namespace Calyx {

	struct ProjectInfo;
	class SceneRegistry;

} // namespace Calyx

namespace CalyxEngine {
	class EngineUICore;
	class SceneManager;
}

namespace Calyx {

	class CALYX_API Application {
	public:
		virtual ~Application() = default;

		virtual void RegisterScenes(SceneRegistry& registry) {
			(void)registry;
		}

		virtual void OnProjectLoaded(const ProjectInfo& project) {
			(void)project;
		}

		virtual void OnSceneManagerReady(CalyxEngine::SceneManager& sceneManager) {
			(void)sceneManager;
		}

		virtual void OnEngineUiReady(CalyxEngine::EngineUICore& engineUi) {
			(void)engineUi;
		}

		virtual void OnInitialize() {}
		virtual void OnUpdate() {}
		virtual void OnRender() {}
		virtual void OnFinalize() {}

		virtual bool ShouldRenderEngineUi() const { return true; }
	};

} // namespace Calyx
