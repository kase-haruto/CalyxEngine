#pragma once
#include <string>
#include <vector>
#include <memory>
#include <Engine/Foundation/Export/CalyxAPI.h>
#include <Engine/Foundation/Utility/Guid/Guid.h>
#include <externals/nlohmann/json.hpp>

class SceneObject;

/*-----------------------------------------------------------------------------------------
 * PrefabSerializer
 * - プレファブシリアライザークラス
 * - シーンオブジェクトのJSON形式での保存・読み込みを担当
 *---------------------------------------------------------------------------------------*/
class CALYX_API PrefabSerializer{
public:
	/*-----------------------------------------------------------------------------------------
	 * SaveOptions
	 * - Prefab保存時の変換方針を指定するデータ構造
	 * - Root Transformの初期化とPrefab由来GUIDの利用有無を管理する
	 *---------------------------------------------------------------------------------------*/
	struct SaveOptions {
		bool resetRootTransform = false;
		bool usePrefabSourceGuids = false;
	};

	/*-----------------------------------------------------------------------------------------
	 * LoadOptions
	 * - Prefab読込時の互換性とGUID再生成方針を指定するデータ構造
	 * - GUID維持、Prefab Asset識別子、未知型の扱いを管理する
	 *---------------------------------------------------------------------------------------*/
	struct LoadOptions {
		bool preserveGuids = false;
		Guid prefabAssetGuid = Guid::Empty();
		bool skipUnknownTypes = false;
	};

	static bool Save(const std::vector<SceneObject*>& roots, const std::string& path);
	static bool Save(const std::vector<SceneObject*>& roots, const std::string& path,
					 const SaveOptions& options);

	static std::vector<std::shared_ptr<SceneObject>> Load(const std::string& path);
	static std::vector<std::shared_ptr<SceneObject>> Load(const std::string& path,
														  const LoadOptions& options);
};
