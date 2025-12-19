#include "SceneObjectFactory.h"

#include <Engine/Objects/3D/Actor/BaseGameObject.h>
#include <Engine/Objects/LightObject/PointLight.h>
#include <Engine/Objects/LightObject/DirectionalLight.h>
#include <Engine/Application/Effects/Particle/Object/ParticleSystemObject.h>
#include <Engine/Graphics/Camera/Base/BaseCamera.h>

std::unordered_map<ObjectType, ObjectFactory::CreatorFunc> ObjectFactory::registry_ = {
	{ ObjectType::GameObject, [](const nlohmann::json& j) {
		auto obj = std::make_unique<BaseGameObject>();
		obj->ApplyConfigFromJson(j);
		return obj;
	}},
	{ ObjectType::Light, [](const nlohmann::json& j) -> std::unique_ptr<SceneObject> {
	if (j.contains("direction")) {
		auto obj = std::make_unique<DirectionalLight>();
		obj->ApplyConfigFromJson(j);
		return obj;
	} else {
		auto obj = std::make_unique<PointLight>();
		obj->ApplyConfigFromJson(j);
		return obj;
	}
	}},
	{ ObjectType::Effect, [] ([[maybe_unused]]const nlohmann::json& j){
		 auto obj = std::make_unique<CalyxEffect::ParticleSystemObject>();
		obj->ApplyConfigFromJson(j);
		return obj;
	}},
	{ ObjectType::Camera, [](const nlohmann::json& j) {
		auto obj = std::make_unique<BaseCamera>();
		obj->ApplyConfigFromJson(j);
		return obj;
	}},

};

std::unique_ptr<SceneObject> ObjectFactory::Create(ObjectType type, const nlohmann::json& j) {
	auto it = registry_.find(type);
	if (it != registry_.end()) {
		return it->second(j);
	}
	return nullptr;
}