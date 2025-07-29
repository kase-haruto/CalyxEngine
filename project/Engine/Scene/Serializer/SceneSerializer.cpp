#include "SceneSerializer.h"

/* ========================================================================
   include space
   ===================================================================== */
#include <Engine/Scene/Context/SceneContext.h>
#include <Engine/Objects/3D/Actor/BaseGameObject.h>
#include <Engine/objects/LightObject/DirectionalLight.h>
#include <Engine/objects/LightObject/PointLight.h>
#include <Engine/Foundation/Json/JsonUtils.h>
#include <Engine/Objects/3D/Actor/Library/SceneObjectLibrary.h>
#include <Engine/Application/Effects/FxSystem.h>
#include <Engine/Application/Effects/Particle/Object/ParticleSystemObject.h>
#include <Engine/Objects/3D/Actor/Registry/SceneObjectRegistry.h>
#include <memory>
#include <unordered_map>

   // -----------------------------------------------------------------------------
   // Save (to file)
   // -----------------------------------------------------------------------------
bool SceneSerializer::Save(const SceneContext& context, const std::string& path){
	auto root = DumpJson(context);
	return JsonUtils::Save(path, root);
}

// -----------------------------------------------------------------------------
// Load (from file)
// -----------------------------------------------------------------------------
bool SceneSerializer::Load(SceneContext& context, const std::string& path){
	nlohmann::json root;
	if (!JsonUtils::Load(path, root)) return false;
	return LoadJson(context, root);
}

// -----------------------------------------------------------------------------
// DumpJson (to memory)
// -----------------------------------------------------------------------------
nlohmann::json SceneSerializer::DumpJson(const SceneContext& context){
	nlohmann::json jObjects = nlohmann::json::array();

	const auto& objects = context.GetObjectLibrary()->GetAllObjectsShared();
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
			jObjects.push_back(std::move(j));
		}
	}

	nlohmann::json root;
	root["version"] = 1;
	root["sceneName"] = context.GetSceneName();
	root["objects"] = std::move(jObjects);
	return root;
}

// -----------------------------------------------------------------------------
// LoadJson (from memory)
// -----------------------------------------------------------------------------
bool SceneSerializer::LoadJson(SceneContext& context, const nlohmann::json& root){
	// objects配列を取得
	nlohmann::json jArray;
	if (root.is_array()){
		// 互換: 旧フォーマット（配列のみ）
		jArray = root;
	} else{
		jArray = root.value("objects", nlohmann::json::array());
		if (root.contains("sceneName")){
			context.SetSceneName(root.value("sceneName", std::string {"scene"}));
		}
	}

	// 既存オブジェクトとサブシステムを初期化（内部リソースは維持）
	context.Clear();

	// ライトの参照を一度クリア（存在すれば）
	if (auto* ll = context.GetLightLibrary()){
		std::shared_ptr<DirectionalLight> emptyDir;
		std::shared_ptr<PointLight> emptyPoint;
		ll->SetDirectionalLight(emptyDir);
		ll->SetPointLight(emptyPoint);
	}

	std::unordered_map<Guid, std::shared_ptr<SceneObject>> guidMap;

	// === 1st pass: 生成・登録 ==================================================
	for (const auto& j : jArray){
		std::string typeName = j.value("type", "");
		if (typeName.empty()) continue;

		auto sp = SceneObjectRegistry::Get().Create(typeName);
		if (!sp) continue;

		if (auto* cfg = dynamic_cast< IConfigurable* >(sp.get())){
			cfg->ApplyConfigFromJson(j);
		}

		// ライブラリへ登録
		context.GetObjectLibrary()->AddObject(sp);

		// サブシステムへ登録
		if (auto d = std::dynamic_pointer_cast< DirectionalLight >(sp)){
			context.GetLightLibrary()->SetDirectionalLight(d);
		} else if (auto p = std::dynamic_pointer_cast< PointLight >(sp)){
			context.GetLightLibrary()->SetPointLight(p);
		} else if (auto fx = std::dynamic_pointer_cast< ParticleSystemObject >(sp)){
			context.GetFxSystem()->AddEmitter(fx);
		}

		Guid guid = j.value("guid", Guid {});
		guidMap[guid] = sp;
	}

	// === 2nd pass: 親子リンク ===================================================
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
