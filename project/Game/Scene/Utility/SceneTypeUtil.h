#pragma once

#include <Engine/Scene/Utility/SceneUtility.h>
#include <Game/Scene/Details/SceneType.h>

#include <cstdint>

namespace GameSceneUtil  {

	CalyxScene::SceneId ToSceneId(SceneType type);
	
}