#pragma once

#include <string>
#include <externals/nlohmann/json.hpp>

struct SceneAsset{
	std::string layout;			//< layoutPath
	std::string controller;		//< controllerクラス
	nlohmann::json ext;			//< 追加
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(SceneAsset, layout, controller, ext)