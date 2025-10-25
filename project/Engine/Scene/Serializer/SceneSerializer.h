#pragma once
/* ========================================================================
/*  include space
/* ===================================================================== */
#include <externals/nlohmann/json.hpp>
#include <string>

class SceneContext;

/// <summary>
/// シーンシリアライズ
/// </summary>
class SceneSerializer {
public:
	// save
	static bool			  Save(const SceneContext& context, const std::string& path);
	static nlohmann::json DumpJson(const SceneContext& context);

	// load
	static bool Load(SceneContext& context, const std::string& path);
	static bool LoadJson(SceneContext& context, const nlohmann::json& root);
};
