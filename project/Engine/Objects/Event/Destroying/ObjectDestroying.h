#pragma once
#include <memory>

class SceneObject;

/**
 * @brief ObjectDestroyingに関するデータを保持する構造体です。
 */
struct ObjectDestroying {
	std::shared_ptr<SceneObject> object;
};
