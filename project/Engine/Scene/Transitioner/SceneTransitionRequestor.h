#pragma once

#include <Game/Scene/Details/SceneType.h>

/* ========================================================================
/* シーン遷移リクエスト
/* ===================================================================== */
class SceneTransitionRequestor {
public:
	virtual void RequestSceneChange(SceneType nextScene) = 0;
	virtual ~SceneTransitionRequestor() = default;
};
