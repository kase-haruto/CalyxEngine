#pragma once

namespace Calyx {

	struct ProjectInfo;
	class SceneRegistry;

	class Application {
	public:
		virtual ~Application() = default;

		virtual void RegisterScenes(SceneRegistry& registry) {
			(void)registry;
		}

		virtual void OnProjectLoaded(const ProjectInfo& project) {
			(void)project;
		}

		virtual void OnInitialize() {}
		virtual void OnUpdate() {}
		virtual void OnRender() {}
		virtual void OnFinalize() {}

		virtual bool ShouldRenderEngineUi() const { return true; }
	};

} // namespace Calyx
