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
#include <Engine/Objects/3D/Actor/Registry/SceneObjectRegistry.h>
#include <memory>

// -----------------------------------------------------------------------------
// Save 
// -----------------------------------------------------------------------------
bool SceneSerializer::Save(const SceneContext& context, const std::string& path){
	const auto& objects = context.GetObjectLibrary()->GetAllObjectsShared();
	nlohmann::json jArray = nlohmann::json::array();

	for (const auto& sp : objects){
		if (!sp || !sp->IsSerializable()) continue;

		if (auto* cfg = dynamic_cast< const IConfigurable* >(sp.get())){
			nlohmann::json j;
			cfg->ExtractConfigToJson(j);

			j["type"] = sp->GetTypeName();
			j["guid"] = sp->GetGuid();
			if (auto parent = sp->GetParent()){
				j["parentGuid"] = parent->GetGuid();
			}
			jArray.push_back(std::move(j));
		}
	}

	return JsonUtils::Save(path, jArray);
}

// -----------------------------------------------------------------------------
// Load 
// -----------------------------------------------------------------------------
bool SceneSerializer::Load(SceneContext& context, const std::string& path){
	nlohmann::json jArray;
	if (!JsonUtils::Load(path, jArray)) return false;

	auto* lib = context.GetObjectLibrary();
	context.Clear();
	lib->Clear();

	std::unordered_map<Guid, std::shared_ptr<SceneObject>> guidMap;

	for (const auto& j : jArray){
		std::string typeName = j.value("type", "");
		if (typeName.empty()) continue;               // 保険

		// 工場でインスタンス生成
		auto sp = SceneObjectRegistry::Get().Create(typeName);
		if (!sp) continue;

		// 設定反映
		if (auto* cfg = dynamic_cast< IConfigurable* >(sp.get())){
			cfg->ApplyConfigFromJson(j);
		}

		//  シーンへ登録
		context.AddEditorObject(sp);

		// サブシステム登録
		if (auto d = std::dynamic_pointer_cast< DirectionalLight >(sp)){
			context.GetLightLibrary()->SetDirectionalLight(d);
		} else if (auto p = std::dynamic_pointer_cast< PointLight >(sp)){
			context.GetLightLibrary()->SetPointLight(p);
		} else if (auto fx = std::dynamic_pointer_cast< ParticleSystemObject >(sp)){
			context.GetFxSystem()->AddEmitter(fx);
		}

		// GUID テーブル格納
		Guid guid = j.value("guid", Guid {});
		guidMap[guid] = sp;
	}

	// === 2nd pass: 親子リンク復元 ====================================================
	for (const auto& j : jArray){
		Guid childGuid = j.value("guid", Guid {});
		Guid parentGuid = j.value("parentGuid", Guid {});

		if (!childGuid.isValid() || !parentGuid.isValid()) continue;

		auto childIt = guidMap.find(childGuid);
		auto parentIt = guidMap.find(parentGuid);
		if (childIt == guidMap.end() || parentIt == guidMap.end()) continue;

		childIt->second->SetParent(parentIt->second);
	}

	return true;
}

