#pragma once

#include <Engine/Objects/3D/Actor/SceneObject.h>
#include <vector>
#include <memory>
#include <algorithm>

class SceneObjectLibrary{
public:
	SceneObjectLibrary() = default;
	~SceneObjectLibrary() = default;

	void AddObject(const std::shared_ptr<SceneObject>& object);

	void RemoveObject(const std::shared_ptr<SceneObject>& obj);

	void Clear();

	template<typename TObject, typename... Args>
	std::shared_ptr<TObject> CreateAndAddObject(Args&&... args);

	std::vector<SceneObject*> GetAllObjects() const;
	const std::vector<std::shared_ptr<SceneObject>>& GetAllObjectsShared() const;

	std::shared_ptr<SceneObject> CreateByTypeName(std::string_view typeName);
private:
	std::vector<std::shared_ptr<SceneObject>> allSceneObjects_;
};

template<typename TObject, typename ...Args>
inline std::shared_ptr<TObject> SceneObjectLibrary::CreateAndAddObject(Args && ...args){
	auto obj = std::make_shared<TObject>(std::forward<Args>(args)...);
	AddObject(obj);
	return obj;
}