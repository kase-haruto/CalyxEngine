#pragma once

#include <Engine/Scene/Details/SceneType.h>

class SceneTransitionRequestor {
public:
	virtual void RequestSceneChange(SceneType nextScene) = 0;
	virtual ~SceneTransitionRequestor() = default;
};
