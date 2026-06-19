#include "SceneTypeUtil.h"

CalyxEngine::SceneId GameSceneUtil::ToSceneId(SceneType type) {
	return static_cast<CalyxEngine::SceneId>(type);
}