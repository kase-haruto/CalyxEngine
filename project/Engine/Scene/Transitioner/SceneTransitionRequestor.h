#pragma once

#include <Game/Scene/Details/SceneType.h>
#include <cstdint>

struct SceneTransitionPayload {
	int32_t score = 0;
};

/* ========================================================================
/* シーン遷移リクエスト
/* ===================================================================== */
class SceneTransitionRequestor {
public:
	virtual void RequestSceneChange(SceneType nextScene) = 0;
	virtual ~SceneTransitionRequestor() = default;
	virtual void RequestSceneChange(SceneType nextScene, const SceneTransitionPayload& payload) = 0;
};
