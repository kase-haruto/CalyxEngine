#pragma once

#include <Engine/Foundation/Export/CalyxAPI.h>
#include <Engine/Foundation/Utility/Guid/Guid.h>
#include <filesystem>

namespace CalyxEngine { class SceneManager; }

namespace Calyx {
	// Compatibility facade for code that receives a registry service. Scene
	// classes are intentionally unsupported; scenes are assets addressed by path/GUID.
	class CALYX_API SceneRegistry {
	public:
		explicit SceneRegistry(CalyxEngine::SceneManager& sceneManager);
		bool OpenScene(const std::filesystem::path& scenePath);
		bool OpenScene(const Guid& sceneAssetGuid);
		void RequestSceneChange(const std::filesystem::path& scenePath);
		void RequestSceneChange(const Guid& sceneAssetGuid);
	private:
		CalyxEngine::SceneManager& sceneManager_;
	};
}
