#pragma once

#include <Engine/Scene/Transitioner/IScenePayload.h>
#include <Engine/Scene/Utility/SceneUtility.h>
#include <cstdint>

namespace CalyxEngine {
	/* ========================================================================
	/* シーン遷移リクエスト
	/* ===================================================================== */
	class ISceneTransitionRequestor {
	public:
		virtual ~ISceneTransitionRequestor()												= default;
		virtual void RequestSceneChange(SceneId nextScene)								= 0;
		virtual void RequestSceneChange(SceneId nextScene, std::unique_ptr<IScenePayload> payload) = 0;
	};

} // namespace CalyxEngine
