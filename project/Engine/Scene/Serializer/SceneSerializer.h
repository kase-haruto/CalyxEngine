#pragma once
/* ========================================================================
/*  include space
/* ===================================================================== */
#include <string>
#include <memory>
#include <vector>
#include <externals/nlohmann/json.hpp>


class SceneContext;

class SceneSerializer{
public:
	static bool Save(const SceneContext& context, const std::string& path);
	static bool Load(SceneContext& context, const std::string& path);

	static nlohmann::json DumpJson(const SceneContext& context);
	static bool LoadJson(SceneContext& context, const nlohmann::json& root);
};
