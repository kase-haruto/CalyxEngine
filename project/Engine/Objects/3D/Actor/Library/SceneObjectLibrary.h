#pragma once

#include <Engine/Objects/3D/Actor/SceneObject.h>
#include <Engine/Foundation/Utility/Guid/Guid.h>

#include <unordered_map>
#include <vector>
#include <memory>
#include <string>
#include <type_traits>

class SceneObjectLibrary{
public:
	SceneObjectLibrary() = default;
	~SceneObjectLibrary() = default;

	/* 追加・削除・クリア --------------------------------------------------*/
	void AddObject(const std::shared_ptr<SceneObject>& object);
	bool RemoveObject(const std::shared_ptr<SceneObject>& object);
	bool RemoveObject(Guid id);
	void Clear();

	/* 検索 --------------------------------------------------------------*/
	std::shared_ptr<SceneObject> Find(Guid id)                       const;
	std::shared_ptr<SceneObject> FindByName(const std::string& name) const;

	template<class TObject>
	std::vector<std::shared_ptr<TObject>> FindByType() const;

	/* 一覧取得 ----------------------------------------------------------*/
	std::vector<SceneObject*>                        GetAllObjectsRaw()   const;
	std::vector<std::shared_ptr<SceneObject>>        GetAllObjectsShared() const;

	bool Contains(const std::shared_ptr<SceneObject>& obj) const;

private:
	std::unordered_map<Guid, std::shared_ptr<SceneObject>> objects_;
};

/* ---------------- テンプレート実装 ---------------------------------------*/
template<class TObject>
std::vector<std::shared_ptr<TObject>> SceneObjectLibrary::FindByType() const{
	static_assert(std::is_base_of_v<SceneObject, TObject>,
				  "TObject must derive from SceneObject");
	std::vector<std::shared_ptr<TObject>> result;
	for (const auto& [id, sp] : objects_){
		if (auto casted = std::dynamic_pointer_cast< TObject >(sp)){
			result.emplace_back(std::move(casted));
		}
	}
	return result;
}
