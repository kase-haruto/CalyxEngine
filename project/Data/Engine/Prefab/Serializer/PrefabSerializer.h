#pragma once
#include <string>
#include <vector>
#include <memory>
#include <Engine/Foundation/Utility/Guid/Guid.h>
#include <externals/nlohmann/json.hpp>

class SceneObject;

/*-----------------------------------------------------------------------------------------
 * PrefabSerializer
 * - プレファブシリアライザークラス
 * - シーンオブジェクトのJSON形式での保存・読み込みを担当
 *---------------------------------------------------------------------------------------*/
class PrefabSerializer{
public:
	struct SaveOptions {
		bool resetRootTransform = false;
		bool usePrefabSourceGuids = false;
	};

	struct LoadOptions {
		bool preserveGuids = false;
		Guid prefabAssetGuid = Guid::Empty();
	};

	static bool Save(const std::vector<SceneObject*>& roots, const std::string& path);
	static bool Save(const std::vector<SceneObject*>& roots, const std::string& path,
					 const SaveOptions& options);

	static std::vector<std::shared_ptr<SceneObject>> Load(const std::string& path);
	static std::vector<std::shared_ptr<SceneObject>> Load(const std::string& path,
														  const LoadOptions& options);
};
