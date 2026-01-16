#pragma once
#include <string>
#include <vector>
#include <memory>
#include <externals/nlohmann/json.hpp>

class SceneObject;

/*-----------------------------------------------------------------------------------------
 * PrefabSerializer
 * - プレファブシリアライザークラス
 * - シーンオブジェクトのJSON形式での保存・読み込みを担当
 *---------------------------------------------------------------------------------------*/
class PrefabSerializer{
public:
	static bool Save(const std::vector<SceneObject*>& roots, const std::string& path);

	static std::vector<std::shared_ptr<SceneObject>> Load(const std::string& path);
};
