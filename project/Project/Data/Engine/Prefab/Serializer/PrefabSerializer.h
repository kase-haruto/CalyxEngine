#pragma once
#include <string>
#include <vector>
#include <memory>
#include <externals/nlohmann/json.hpp>

class SceneObject;

class PrefabSerializer{
public:
	static bool Save(const std::vector<SceneObject*>& roots, const std::string& path);

	static std::vector<std::shared_ptr<SceneObject>> Load(const std::string& path);
};
