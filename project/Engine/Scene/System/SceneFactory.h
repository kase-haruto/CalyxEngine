#pragma once

// engine
#include <Engine/Scene/Base/IScene.h>

// game
#include <Game/Scene/Details/SceneType.h>

// lib
#include <memory>

class SceneFactory{
public:
	static std::unique_ptr<IScene> CreateScene(SceneType sceneType);
};
