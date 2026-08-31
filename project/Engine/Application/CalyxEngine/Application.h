#pragma once

#include <Engine/Foundation/Export/CalyxAPI.h>

namespace Calyx {

	struct ProjectInfo;
} // namespace Calyx

namespace CalyxEngine {
	class EngineUICore;
	class SceneManager;
}

namespace Calyx {

	class CALYX_API Application {
	public:
		virtual ~Application() = default;

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

		// Game applications normally discover a nearby project when launched
		// without arguments. Editors can opt out to show their project browser.
		virtual bool ShouldAutoDiscoverProject() const { return true; }
	};

} // namespace Calyx
