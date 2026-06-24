#pragma once

#include <Engine/Foundation/Export/CalyxAPI.h>
#include <cstddef>
#include <string>
#include <unordered_map>
#include <vector>
#include <stdexcept>
#include <memory>

#include <Engine/Objects/3D/Actor/SceneObject.h>

using SceneObjectFactory = std::shared_ptr<SceneObject>(*)();

template<class T>
std::shared_ptr<SceneObject> CreateSceneObject() {
	return std::make_shared<T>();
}

struct SceneObjectClassDesc {
	std::string typeName;
	std::string displayName;
	ObjectType	 objectType = ObjectType::None;
	std::string iconPath;
	bool		 placeable = false;
	bool		 prefabEditable = false;
	bool		 prefabRoot = false;
	SceneObjectFactory factory = nullptr;
};


/* ========================================================================
/*		jsonの文字列からインスタンスを作成するため
/* ===================================================================== */
class SceneObjectRegistry{
public:
	CALYX_API static SceneObjectRegistry& Get();

	/// <summary>
	/// 文字列名とそれを生成するctorを保存
	/// </summary>
	/// <param name="typeName"></param>
	/// <param name="ctor"></param>
	CALYX_API void Register(std::string_view typeName, SceneObjectFactory factory);
	CALYX_API void Register(
		const char* typeName,
		const char* displayName,
		ObjectType objectType,
		const char* iconPath,
		bool placeable,
		bool prefabEditable,
		bool prefabRoot,
		SceneObjectFactory factory);
	CALYX_API void Register(const SceneObjectClassDesc& desc);

	/// <summary>
	/// 登録済みの名前に対応するオブジェクトを生成
	/// </summary>
	/// <param name="typeName"></param>
	/// <returns></returns>
	CALYX_API std::shared_ptr<SceneObject> Create(std::string_view typeName) const;

	/// <summary>
	/// 登録済みオブジェクト名を一覧で返す
	/// </summary>
	/// <returns></returns>
	CALYX_API std::vector<std::string> ListTypes() const;
	CALYX_API std::vector<SceneObjectClassDesc const*> ListPlaceableTypes() const;
	CALYX_API std::vector<SceneObjectClassDesc const*> ListPrefabEditableTypes() const;
	CALYX_API std::vector<SceneObjectClassDesc const*> ListPrefabRootTypes() const;
	CALYX_API const SceneObjectClassDesc* Find(std::string_view typeName) const;
	CALYX_API std::size_t GetRevision() const;

private:
	/// <summary>
	/// オブジェクト登録テーブル
	/// </summary>
	std::unordered_map<std::string, SceneObjectClassDesc> table_;
	std::size_t revision_ = 0;
};

// 登録マクロ
#define REGISTER_SCENE_OBJECT(T) \
	namespace { const bool _rg_##T = []{ \
		SceneObjectRegistry::Get().Register(#T, &CreateSceneObject<T>); \
		return true; }(); }
