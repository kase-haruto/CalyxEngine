#pragma once

// engine
#include <Engine/Scene/Base/IScene.h>

// lib
#include <memory>

class SceneFactory{
public:
	static std::unique_ptr<IScene> CreateScene(SceneType sceneType);
};
