#pragma once

#include <Engine/Scene/Context/SceneContext.h>
#include <Engine/Objects/3D/Actor/SceneObject.h>

#include <vector>
#include <memory>

template<typename T, typename... Args>
std::shared_ptr<T> CreateAndAddObject(SceneContext* context, Args&&... args){
	return CreateAndAddObject<T>(context->GetObjectLibrary(), std::forward<Args>(args)...);
}

template<typename T, typename... Args>
std::shared_ptr<T> CreateAndAddObject(SceneObjectLibrary* library, Args&&... args){
	static_assert(std::is_base_of<SceneObject, T>::value, "T must derive from SceneObject");

	auto obj = std::make_shared<T>(std::forward<Args>(args)...);
	obj->Initialize();

	if (library){
		library->AddObject(obj);
	}

	return obj;
}
