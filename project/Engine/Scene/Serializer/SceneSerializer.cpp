#include "SceneSerializer.h"

/* ========================================================================
/*  include space
/* ===================================================================== */
#include <Engine/Scene/Context/SceneContext.h>
#include <Engine/Objects/3D/Actor/BaseGameObject.h>
#include <Engine/objects/LightObject/PointLight.h>
#include <Engine/Foundation/Json/JsonUtils.h>
#include <Engine/Objects/3D/Actor/Library/SceneObjectLibrary.h>
#include <Engine/Application/Effects/FxSystem.h>
#include <Engine/Application/Effects/Particle/Object/ParticleSystemObject.h>
#include <Engine/Scene/Utirity/SceneUtility.h>
#include <memory>

bool SceneSerializer::Save(const SceneContext& context, const std::string& path){
	const auto& objects = context.GetObjectLibrary()->GetAllObjects();
	nlohmann::json jArray = nlohmann::json::array();

	for (const auto& obj : objects){
		if (!obj || !obj->IsSerializable()) continue;

		if (auto* config = dynamic_cast< const IConfigurable* >(obj)){
			nlohmann::json j;
			config->ExtractConfigToJson(j);
			j["objectType"] = static_cast< int >(obj->GetObjectType());
			jArray.push_back(j);
		}
	}

	return JsonUtils::Save(path, jArray);
}

bool SceneSerializer::Load(SceneContext& context, const std::string& path){
	nlohmann::json jArray;
	if (!JsonUtils::Load(path, jArray)) return false;

	auto* lib = context.GetObjectLibrary();
	context.Clear();
	lib->Clear();

	// --- 1st pass: オブジェクトをすべて生成 ---
	std::unordered_map<Guid, SceneObject*> guidToObjectMap;

	for (const auto& j : jArray){
		int type = j.value("objectType", -1);
		std::string name = j.value("name", "UnnamedObject");

		SceneObject* createdObj = nullptr;

		switch (static_cast< ObjectType >(type)){
			case ObjectType::GameObject:
			{
				std::string modelName = j.value("modelName", "debugCube.obj");

				auto ptr = std::make_shared<BaseGameObject>(modelName, name);
				ptr->ConfigurableObject<BaseGameObjectConfig>::ApplyConfigFromJson(j);

				context.AddEditorObject(ptr);

				createdObj = ptr.get();
				break;
			}
			case ObjectType::Light:
			{
				if (j.contains("direction")){
					auto ptr = CreateAndAddObject<DirectionalLight>(&context, "DirectionalLightName");
					ptr->ApplyConfigFromJson(j);

					context.GetLightLibrary()->SetDirectionalLight(ptr);
					createdObj = ptr.get();
				} else{
					auto ptr = CreateAndAddObject<PointLight>(&context, "PointLightName");
					ptr->ApplyConfigFromJson(j);

					context.GetLightLibrary()->SetPointLight(ptr); 
					createdObj = ptr.get();
				}
				break;
			}
			case ObjectType::ParticleSystem:
			{
				auto ptr = CreateAndAddObject<ParticleSystemObject>(&context, "ParticleSystem");
				ptr->ApplyConfigFromJson(j);

				context.GetFxSystem()->AddEmitter(ptr);

				createdObj = ptr.get();
				break;
			}
			default:
				continue;
		}

		// guid登録
		if (createdObj){
			Guid guid = j.value("guid", Guid {});
			guidToObjectMap[guid] = createdObj;
		}
	}

	// --- 2nd pass: 親子関係を復元 ---
	for (const auto& j : jArray){
		Guid childGuid = j.value("guid", Guid {});
		Guid parentGuid = j.value("parentGuid", Guid {});

		if (parentGuid.isValid() && childGuid.isValid()){
			auto childIt = guidToObjectMap.find(childGuid);
			auto parentIt = guidToObjectMap.find(parentGuid);

			if (childIt != guidToObjectMap.end() && parentIt != guidToObjectMap.end()){
				SceneObject* child = childIt->second;
				SceneObject* parent = parentIt->second;

				child->SetParent(parent);  // ← これが親子関係を設定する関数
			}
		}
	}

	return true;
}
