#pragma once

#include <Engine/Scene/Transitioner/IScenePayload.h>
#include <Engine/Foundation/Utility/Guid/Guid.h>
#include <filesystem>

namespace CalyxEngine {
	/* ========================================================================
	/* シーン遷移リクエスト
	/* ===================================================================== */
	class ISceneTransitionRequestor {
	public:
		virtual ~ISceneTransitionRequestor()												= default;
		virtual void RequestSceneChange(const std::filesystem::path& scenePath) = 0;
		virtual void RequestSceneChange(const std::filesystem::path& scenePath, std::unique_ptr<IScenePayload> payload) = 0;
		virtual void RequestSceneChange(const Guid& sceneAssetGuid) = 0;
		virtual void RequestSceneChange(const Guid& sceneAssetGuid, std::unique_ptr<IScenePayload> payload) = 0;
	};

} // namespace CalyxEngine
