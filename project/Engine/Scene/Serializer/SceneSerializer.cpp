#include "SceneSerializer.h"

/* ========================================================================
   include space
   ===================================================================== */
#include <Engine/Application/Effects/FxSystem.h>
#include <Engine/Application/Effects/Particle/Object/ParticleSystemObject.h>
#include <Engine/Foundation/Json/JsonUtils.h>
#include <Engine/Foundation/Serialization/SerializableObject.h>
#include <Engine/Objects/3D/Actor/Library/SceneObjectLibrary.h>
#include <Engine/Objects/3D/Actor/Registry/SceneObjectRegistry.h>
#include <Engine/Scene/Context/SceneContext.h>
#include <Engine/objects/LightObject/DirectionalLight.h>
#include <Engine/objects/LightObject/PointLight.h>
#include <memory>
#include <unordered_map>
#include <unordered_set>

using namespace CalyxEngine;

namespace {
	bool HasInlineConfigData(const nlohmann::json& j) {
		static const std::unordered_set<std::string> kMetadataKeys = {
			"type",
			"guid",
			"prefabAssetGuid",
			"prefabSourceGuid",
			"parentGuid",
			"configPath",
			"serializableParams",
		};

		for(auto it = j.begin(); it != j.end(); ++it) {
			if(!kMetadataKeys.contains(it.key())) {
				return true;
			}
		}
		return false;
	}

	void ApplySceneConfig(SceneObject& object, const nlohmann::json& j) {
		auto* cfg = dynamic_cast<IConfigurable*>(&object);
		if(!cfg) return;

		if(j.contains("configPath")) {
			const std::string cfgPath = j.at("configPath").get<std::string>();
			object.SetConfigPath(cfgPath);

			nlohmann::json jCfg;
			if(JsonUtils::Load(cfgPath, jCfg)) {
				cfg->ApplyConfigFromJson(jCfg);
			}
		}

		if(!j.contains("configPath") || HasInlineConfigData(j)) {
			cfg->ApplyConfigFromJson(j);
		}
	}
}

// -----------------------------------------------------------------------------
// Save (to file)
// -----------------------------------------------------------------------------
bool SceneSerializer::Save(const SceneContext& context, const std::string& path) {
	auto root = DumpJson(context);
	return JsonUtils::Save(path, root);
}

// -----------------------------------------------------------------------------
// Load (from file)
// -----------------------------------------------------------------------------
bool SceneSerializer::Load(SceneContext& context, const std::string& path) {
	nlohmann::json root;
	if(!JsonUtils::Load(path, root)) return false;
	return LoadJson(context, root);
}

// -----------------------------------------------------------------------------
// DumpJson (to memory)
// -----------------------------------------------------------------------------
nlohmann::json SceneSerializer::DumpJson(const SceneContext& context) {
	nlohmann::json jObjects = nlohmann::json::array();

	const auto& objects = context.GetObjectLibrary()->GetAllObjectsShared();
	for(const auto& sp : objects) {
		if(!sp || !sp->IsSerializable()) continue;

		// FX系のオブジェクトは保存対象から除外（ロード時に再生成されるため）
		if(sp->GetObjectType() == ObjectType::Effect) continue;

		// IConfigurable を持つものだけ出力対象
		if(auto* cfg = dynamic_cast<const IConfigurable*>(sp.get())) {
			nlohmann::json jOne;

			// ---- 基本メタ ----
			jOne["type"] = sp->GetObjectClassName();
			jOne["guid"] = sp->GetGuid();
			if(sp->GetPrefabAssetGuid().isValid()) {
				jOne["prefabAssetGuid"] = sp->GetPrefabAssetGuid();
			}
			if(sp->GetPrefabSourceGuid().isValid()) {
				jOne["prefabSourceGuid"] = sp->GetPrefabSourceGuid();
			}
			if(auto parent = sp->GetParent()) {
				jOne["parentGuid"] = parent->GetGuid();
			}

			nlohmann::json jInline;
			cfg->ExtractConfigToJson(jInline);
			for(auto it = jInline.begin(); it != jInline.end(); ++it) {
				jOne[it.key()] = it.value();
			}

			// ---- 外部設定パスの有無で分岐（SceneObject が保持）----
			const std::string& cfgPath = sp->GetConfigPath();
			if(!cfgPath.empty()) {
				// 個別JSONへ書き出す
				// シーンにはパスのみ記録
				jOne["configPath"] = cfgPath;
			} else {
				// 設定を内包
				nlohmann::json jInlineFallback;
				cfg->ExtractConfigToJson(jInlineFallback);
				// 内包データを jOne にマージ
				for(auto it = jInlineFallback.begin(); it != jInlineFallback.end(); ++it) {
					jOne[it.key()] = it.value();
				}
			}

			nlohmann::json serializableParams;
			sp->ExtractSerializableParamsToJson(serializableParams);
			if(!serializableParams.empty()) {
				jOne["serializableParams"] = std::move(serializableParams);
			}

			jObjects.push_back(std::move(jOne));
		}
	}

	nlohmann::json root;
	root["version"]	  = 1;
	root["sceneName"] = context.GetSceneName();
	root["objects"]	  = std::move(jObjects);
	return root;
}

// -----------------------------------------------------------------------------
// LoadJson (from memory)
// -----------------------------------------------------------------------------
bool SceneSerializer::LoadJson(SceneContext&		 context,
							   const nlohmann::json& root) {
	// ---------- 配列取得（旧形式配慮） ----------
	nlohmann::json jArray;
	if(root.is_array()) {
		jArray = root; // 旧：直接配列
	} else {
		jArray = root.value("objects", nlohmann::json::array());
	}

	if(root.contains("sceneName")) context.SetSceneName(root.value("sceneName", "scene"));

	// ---------- 既存クリア ----------
	context.Clear();

	// Light & Camera を一旦無効化
	if(auto* ll = context.GetLightLibrary()) {
		ll->SetDirectionalLight({});
		ll->SetPointLight({});
	}
	if(auto* cm = context.GetCameraMgr()) {
		cm->SetMainCamera({});
		cm->SetDebugCamera({});
	}

	std::unordered_map<Guid, std::shared_ptr<SceneObject>> guidMap;

	// ---------- 生成 & 設定適用 & サブシステム登録 ----------
	for(const auto& j : jArray) {
		std::string typeName = j.value("type", "");
		if(typeName.empty()) continue;

		const nlohmann::json* paramOverrides = j.contains("serializableParams")
			? &j.at("serializableParams")
			: nullptr;
		SerializableObject::BeginPendingCapture();
		auto sp = SceneObjectRegistry::Get().Create(typeName);
		if(!sp) {
			SerializableObject::EndPendingCapture(nullptr, nullptr);
			continue;
		}
		sp->AdoptPendingSerializableParamCapture(paramOverrides);

		ApplySceneConfig(*sp, j);
		if(false) {
			auto* cfg = dynamic_cast<IConfigurable*>(sp.get());
			// onfigPath があるなら外部JSONを優先
			if(j.contains("configPath")) {
				const std::string cfgPath = j.at("configPath").get<std::string>();
				sp->SetConfigPath(cfgPath); // SceneObject に保持（save は SceneSerializer 側でのみ実施）

				nlohmann::json jCfg;
				if(JsonUtils::Load(cfgPath, jCfg)) {
					cfg->ApplyConfigFromJson(jCfg);
				} else {
					// フォールバック
					cfg->ApplyConfigFromJson(j);
				}
			} else {
				// 内包をそのまま適用
				cfg->ApplyConfigFromJson(j);
			}
		}

		Guid guid = j.value("guid", Guid{});
		if(!guid.isValid()) {
			guid = sp->GetGuid();
		}
		sp->SetGuid(guid);

		// ライブラリへ登録
		context.GetObjectLibrary()->AddObject(sp);
		sp->BeginSerializableParamCapture(paramOverrides);
		sp->Initialize();
		sp->EndSerializableParamCapture();
		ApplySceneConfig(*sp, j);
		sp->SetGuid(guid);
		if(false) {
			auto* cfg = dynamic_cast<IConfigurable*>(sp.get());
			if(j.contains("configPath")) {
				const std::string cfgPath = j.at("configPath").get<std::string>();
				sp->SetConfigPath(cfgPath);

				nlohmann::json jCfg;
				if(JsonUtils::Load(cfgPath, jCfg)) {
					cfg->ApplyConfigFromJson(jCfg);
				} else {
					cfg->ApplyConfigFromJson(j);
				}
			} else {
				cfg->ApplyConfigFromJson(j);
			}
		}

		const Guid prefabAssetGuid = j.value("prefabAssetGuid", Guid{});
		const Guid prefabSourceGuid = j.value("prefabSourceGuid", Guid{});
		if(prefabAssetGuid.isValid() && prefabSourceGuid.isValid()) {
			sp->SetPrefabLink(prefabAssetGuid, prefabSourceGuid);
		}

		// サブシステムへ橋渡し
		if(auto dir = std::dynamic_pointer_cast<DirectionalLight>(sp)) {
			context.GetLightLibrary()->SetDirectionalLight(dir);
		} else if(auto pt = std::dynamic_pointer_cast<PointLight>(sp)) {
			context.GetLightLibrary()->AddPointLight(pt);
		} else if(auto camDbg = std::dynamic_pointer_cast<DebugCamera>(sp)) {
			context.GetCameraMgr()->SetDebugCamera(camDbg);
		} else if(auto camMain = std::dynamic_pointer_cast<Camera3d>(sp)) {
			context.GetCameraMgr()->SetMainCamera(camMain);
		}

		// GUID
		guidMap[guid] = sp;
	}

	// ---------- 親子リンク ----------
	for(const auto& j : jArray) {
		Guid child	= j.value("guid", Guid{});
		Guid parent = j.value("parentGuid", Guid{});
		if(!child.isValid() || !parent.isValid()) continue;

		auto cIt = guidMap.find(child);
		auto pIt = guidMap.find(parent);
		if(cIt != guidMap.end() && pIt != guidMap.end()) {
			auto& childTransform = cIt->second->GetWorldTransform();
			cIt->second->SetParent(pIt->second, childTransform.inheritScale);
		}
	}
	return true;
}
