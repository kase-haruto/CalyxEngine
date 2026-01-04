#pragma once
#include <memory>

class SceneObject;

struct ObjectDestroying {
	std::shared_ptr<SceneObject> object;
};