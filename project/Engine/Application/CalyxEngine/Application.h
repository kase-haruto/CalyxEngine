#pragma once

namespace Calyx {

	class SceneRegistry;

	class Application {
	public:
		virtual ~Application() = default;

		virtual void RegisterScenes(SceneRegistry& registry) {
			(void)registry;
		}

		virtual void OnInitialize() {}
		virtual void OnUpdate() {}
		virtual void OnRender() {}
		virtual void OnFinalize() {}
	};

} // namespace Calyx
