#pragma once

#include <Engine/Foundation/Export/CalyxAPI.h>
#include <cstddef>
#include <string>
#include <unordered_map>
#include <vector>
#include <stdexcept>
#include <memory>
#include <cstdint>

#include <Engine/Objects/3D/Actor/SceneObject.h>

using SceneObjectFactory = std::shared_ptr<SceneObject>(*)();

enum class ObjectRegistrationFlags : uint32_t {
	None = 0,
	RuntimeType = 1 << 0,
	SceneObject = 1 << 1,
	EditorSpawn = 1 << 2,
	Prefab = 1 << 3,
};

inline ObjectRegistrationFlags operator|(ObjectRegistrationFlags lhs, ObjectRegistrationFlags rhs) {
	return static_cast<ObjectRegistrationFlags>(
		static_cast<uint32_t>(lhs) | static_cast<uint32_t>(rhs));
}

inline ObjectRegistrationFlags& operator|=(ObjectRegistrationFlags& lhs, ObjectRegistrationFlags rhs) {
	lhs = lhs | rhs;
	return lhs;
}

inline bool HasObjectRegistrationFlag(ObjectRegistrationFlags flags, ObjectRegistrationFlags flag) {
	return (static_cast<uint32_t>(flags) & static_cast<uint32_t>(flag)) != 0;
}

template<class T>
std::shared_ptr<SceneObject> CreateSceneObject() {
	return std::make_shared<T>();
}

/**
 * @brief SceneObjectClassDescに関するデータを保持する構造体です。
 */
struct SceneObjectClassDesc {
	std::string typeName;
	std::string displayName;
	ObjectRegistrationFlags flags = ObjectRegistrationFlags::None; //< オブジェクト型の登録属性
	ObjectType	 objectType = ObjectType::None;
	std::string iconPath;
	bool		 placeable = false;
	bool		 prefabEditable = false;
	bool		 prefabRoot = false;
	bool		 sceneSerializable = false;
	bool		 prefabSerializable = false;
	SceneObjectFactory factory = nullptr;
};


/*-----------------------------------------------------------------------------------------
 * SceneObjectRegistry
 * - オブジェクト型の生成関数と登録属性を管理する
 * - 各オブジェクトのライフタイムやシーンインスタンスは管理しない
 *---------------------------------------------------------------------------------------*/
/**
 * @brief SceneObjectRegistryの機能を提供するクラスです。
 */
class SceneObjectRegistry{
public:
	CALYX_API static SceneObjectRegistry& Get();

	/**
	 * \brief ランタイム生成用の型名とFactoryを登録する
	 * \param typeName 登録する型名
	 * \param factory インスタンス生成関数
	 */
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
	CALYX_API void Register(
		const char* typeName,
		const char* displayName,
		ObjectType objectType,
		const char* iconPath,
		bool placeable,
		bool prefabEditable,
		bool prefabRoot,
		bool sceneSerializable,
		bool prefabSerializable,
		SceneObjectFactory factory);
	CALYX_API void Register(const SceneObjectClassDesc& desc);
	CALYX_API void RegisterAlias(std::string_view aliasTypeName, std::string_view canonicalTypeName);

	/**
	 * \brief 登録済みの名前に対応するオブジェクトを生成する
	 * \param typeName 生成対象の型名
	 * \return 生成したSceneObject
	 */
	CALYX_API std::shared_ptr<SceneObject> Create(std::string_view typeName) const;

	/**
	 * \brief 登録済みオブジェクト名を一覧で取得する
	 * \return 登録済み型名一覧
	 */
	CALYX_API std::vector<std::string> ListTypes() const;
	CALYX_API std::vector<SceneObjectClassDesc const*> ListPlaceableTypes() const;
	CALYX_API std::vector<SceneObjectClassDesc const*> ListPrefabEditableTypes() const;
	CALYX_API std::vector<SceneObjectClassDesc const*> ListPrefabRootTypes() const;
	CALYX_API const SceneObjectClassDesc* Find(std::string_view typeName) const;
	/**
	 * \brief 指定した型がシーン保存対象かを取得する
	 * \param typeName 判定対象の型名
	 * \return シーン保存対象の場合はtrue
	 */
	CALYX_API bool IsSceneSerializable(std::string_view typeName) const;
	/**
	 * \brief 指定した型がPrefab保存対象かを取得する
	 * \param typeName 判定対象の型名
	 * \return Prefab保存対象の場合はtrue
	 */
	CALYX_API bool IsPrefabSerializable(std::string_view typeName) const;
	CALYX_API std::size_t GetRevision() const;

private:
	const SceneObjectClassDesc* FindExact(std::string_view typeName) const;

	std::unordered_map<std::string, SceneObjectClassDesc> table_;
	std::unordered_map<std::string, std::string> aliases_;
	std::size_t revision_ = 0;
};

// 登録マクロ
#define REGISTER_SCENE_OBJECT(T) \
	namespace { const bool _rg_##T = []{ \
		SceneObjectRegistry::Get().Register(#T, #T, ObjectType::None, "", false, false, false, true, true, &CreateSceneObject<T>); \
		return true; }(); }
