#include "SceneFactory.h"

//scene
#include <Engine/Scene/Game/GameScene.h>
#include <Engine/Scene/Test/TestScene.h>
#include <Engine/Scene/Title/TitleScene.h>

std::unique_ptr<IScene> SceneFactory::CreateScene(SceneType sceneType){
	switch (sceneType){
		case SceneType::PLAY:
			return std::make_unique<GameScene>();
		case SceneType::TEST:
			return std::make_unique<TestScene>();
		case SceneType::TITLE:
			return std::make_unique< TitleScene>();
		default:
			return nullptr;
	}
}