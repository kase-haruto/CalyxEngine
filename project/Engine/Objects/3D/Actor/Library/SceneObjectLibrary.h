#pragma once

#include <Engine/Objects/3D/Actor/SceneObject.h>
#include <Engine/Foundation/Utility/Guid/Guid.h>

#include <unordered_map>
#include <vector>
#include <memory>
#include <string>
#include <type_traits>
#include <algorithm>

/* ========================================================================
/*		シーンオブジェクトを一括保有するクラス
/* ===================================================================== */
class SceneObjectLibrary{
public:
	SceneObjectLibrary() = default;
	~SceneObjectLibrary() = default;

	/* 追加・削除・クリア --------------------------------------------------*/
	void AddObject(const std::shared_ptr<SceneObject>& object);
	bool RemoveObject(const std::shared_ptr<SceneObject>& object);
	bool RemoveObject(Guid id);
	void Clear(); // 子孫含めて Remove イベントを正しく発火

	/* 検索 --------------------------------------------------------------*/
	
	/// <summary>
	/// guid空検索
	/// </summary>
	/// <param name="id"></param>
	/// <returns></returns>
	std::shared_ptr<SceneObject> Find(Guid id) const;

	/// <summary>
	/// 名前から検索
	/// </summary>
	/// <param name="name"></param>
	/// <returns></returns>
	std::shared_ptr<SceneObject> FindByName(const std::string& name) const;

	/// <summary>
	/// タイプから探す
	/// </summary>
	/// <typeparam name="TObject"></typeparam>
	/// <returns></returns>
	template <class TObject>
	std::vector<std::shared_ptr<TObject>> FindByType() const;

	/* 一覧取得 ----------------------------------------------------------*/
	
	/// <summary>
	/// シーンオブジェクトの生ポインタを返す
	/// </summary>
	/// <returns></returns>
	std::vector<SceneObject*> GetAllObjectsRaw() const;

	/// <summary>
	/// すべてのシーンオブジェクトのshardPtrを返す
	/// </summary>
	/// <returns></returns>
	std::vector<std::shared_ptr<SceneObject>> GetAllObjectsShared() const;

	/// <summary>
	/// シーンオブジェクトがライブラリに含まれるかを返す
	/// </summary>
	/// <param name="obj"></param>
	/// <returns> 含まれる場合true </returns>
	bool Contains(const std::shared_ptr<SceneObject>& obj) const;
	bool Contains(Guid id) const{ return objects_.contains(id); }

private:
	/// <summary>
	/// ツリーを集める
	/// </summary>
	/// <param name="node"></param>
	/// <param name="out"></param>
	static void GatherSubtreePostorder(
		const std::shared_ptr<SceneObject>& node,
		std::vector<std::shared_ptr<SceneObject>>& out);

	/// <summary>
	/// サブツリーを後行順で外す
	/// </summary>
	/// <param name="root"></param>
	void RemoveSubtreePostorder(const std::shared_ptr<SceneObject>& root);

private:
	std::unordered_map<Guid, std::shared_ptr<SceneObject>> objects_;
};

/* ---------------- テンプレート実装 ---------------------------------------*/
template <class TObject>
std::vector<std::shared_ptr<TObject>> SceneObjectLibrary::FindByType() const{
	static_assert(std::is_base_of_v<SceneObject, TObject>,
				  "TObject must derive from SceneObject");
	std::vector<std::shared_ptr<TObject>> result;
	result.reserve(objects_.size());
	for (const auto& [id, sp] : objects_){
		if (auto casted = std::dynamic_pointer_cast<TObject>(sp)){ result.emplace_back(std::move(casted)); }
	}
	return result;
}