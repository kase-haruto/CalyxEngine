#include "SceneObjectFactory.h"

#include <Engine/Objects/3D/Actor/BaseGameObject.h>
#include <Engine/Objects/LightObject/PointLight.h>
#include <Engine/Objects/LightObject/DirectionalLight.h>
#include <Engine/Application/Effects/Particle/Object/ParticleSystemObject.h>
#include <Engine/Graphics/Camera/Base/BaseCamera.h>

std::unordered_map<ObjectType, ObjectFactory::CreatorFunc> ObjectFactory::registry_ = {
	// ObjectTypeごとに生成とConfig適用をまとめ、呼び出し側の型分岐をFactoryへ集約する。
	{ ObjectType::GameObject, [](const nlohmann::json& j) {
		auto obj = std::make_unique<BaseGameObject>();
		obj->ApplyConfigFromJson(j);
		return obj;
	}},
	{ ObjectType::Light, [](const nlohmann::json& j) -> std::unique_ptr<SceneObject> {
	// 旧Light Configは共通ObjectTypeのため、direction Keyの有無から派生型を判定する。
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
		 auto obj = std::make_unique<CalyxEngine::ParticleSystemObject>();
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
	// 未登録Typeではnullを返し、呼び出し側が互換性のないScene要素を安全に無視できるようにする。
	auto it = registry_.find(type);
	if (it != registry_.end()) {
		return it->second(j);
	}
	return nullptr;
}
