#include "GameApplication.h"

#include <CalyxEngine/SceneRegistry.h>
#include <Demo/Scene/DemoScene/DemoScene.h>
#include <Demo/Scene/Utility/SceneTypeUtil.h>

void GameApplication::RegisterScenes(Calyx::SceneRegistry& registry) {
	registry.AddScene<DemoScene>(GameSceneUtil::ToSceneId(SceneType::DEMO));
	registry.SetStartupScene(GameSceneUtil::ToSceneId(SceneType::DEMO));
}

void GameApplication::OnInitialize() {}

void GameApplication::OnUpdate() {}

void GameApplication::OnRender() {}

void GameApplication::OnFinalize() {}
