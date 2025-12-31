#include "SceneTypeUtil.h"

CalyxScene::SceneId GameSceneUtil::ToSceneId(SceneType type) {
	return static_cast<CalyxScene::SceneId>(type);
}