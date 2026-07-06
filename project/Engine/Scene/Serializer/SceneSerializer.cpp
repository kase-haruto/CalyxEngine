#include "SceneSerializer.h"

/* ========================================================================
   include space
   ===================================================================== */
#include <CalyxEngine/Project.h>
#include <Engine/Application/Effects/FxSystem.h>
#include <Engine/Application/Effects/Particle/Object/ParticleSystemObject.h>
#include <Engine/Foundation/Json/JsonUtils.h>
#include <Engine/Foundation/Log/EngineLogger.h>
#include <Engine/Foundation/Serialization/SerializableObject.h>
#include <Engine/Objects/3D/Actor/BaseGameObject.h>
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
			"boneParentBindings",
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
			} else {
				EngineLogger::GetInstance().Add(
					LogLevel::Warning,
					LogCategory::Asset,
					"Scene object config could not be loaded: " + cfgPath,
					"SceneSerializer");
			}
		}

		if(!j.contains("configPath") || HasInlineConfigData(j)) {
			cfg->ApplyConfigFromJson(j);
		}
	}

	nlohmann::json BuildBoneParentBindingsJson(
		const BaseGameObject& owner,
		const std::vector<std::shared_ptr<SceneObject>>& objects) {
		nlohmann::json bindings = nlohmann::json::array();

		for(const auto& binding : owner.GetBoneParentBindings()) {
			if(!binding.target || binding.boneName.empty()) continue;

			for(const auto& target : objects) {
				if(!target) continue;
				if(&target->GetWorldTransform() != binding.target) continue;

				bindings.push_back(nlohmann::json{
					{"targetGuid", target->GetGuid()},
					{"boneName", binding.boneName},
					{"inheritScale", binding.inheritScale}});
				break;
			}
		}

		return bindings;
	}
}

// -----------------------------------------------------------------------------
// Save (to file)
// -----------------------------------------------------------------------------
bool SceneSerializer::Save(const SceneContext& context, const std::string& path) {
	auto root = DumpJson(context);
	const std::string resolvedPath = Calyx::ResolveAssetPath(path).generic_string();
	const bool succeeded = JsonUtils::Save(resolvedPath, root);
	EngineLogger::GetInstance().Add(
		succeeded ? LogLevel::Trace : LogLevel::Error,
		LogCategory::Editor,
		(succeeded ? "Scene serialized: " : "Scene serialization failed: ") + resolvedPath,
		"SceneSerializer");
	return succeeded;
}

// -----------------------------------------------------------------------------
// Load (from file)
// -----------------------------------------------------------------------------
bool SceneSerializer::Load(SceneContext& context, const std::string& path) {
	nlohmann::json root;
	const std::string resolvedPath = Calyx::ResolveAssetPath(path).generic_string();
	if(!JsonUtils::Load(resolvedPath, root)) {
		EngineLogger::GetInstance().Add(LogLevel::Error, LogCategory::Editor, "Scene file could not be read: " + resolvedPath, "SceneSerializer");
		return false;
	}
	const bool succeeded = LoadJson(context, root);
	EngineLogger::GetInstance().Add(
		succeeded ? LogLevel::Trace : LogLevel::Error,
		LogCategory::Editor,
		(succeeded ? "Scene deserialized: " : "Scene deserialization failed: ") + resolvedPath,
		"SceneSerializer");
	return succeeded;
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
			jOne["type"] = std::string(sp->GetTypeName());
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
			if(auto* owner = dynamic_cast<const BaseGameObject*>(sp.get())) {
				auto bindings = BuildBoneParentBindingsJson(*owner, objects);
				if(!bindings.empty()) {
					jOne["boneParentBindings"] = std::move(bindings);
				}
			}

			jObjects.push_back(std::move(jOne));
		}
	}

	nlohmann::json root;
	root["version"]	  = 2;
	root["sceneName"] = context.GetSceneName();
	// SceneObjectへ属さない設定は独立したsettingsセクションへ保存する。
	// これによりCollision以外のシーン設定も同じ形式で追加できる。
	root["settings"] = context.GetSettings().ToJson();
	root["objects"]	  = std::move(jObjects);
	return root;
}

// -----------------------------------------------------------------------------
// LoadJson (from memory)
// -----------------------------------------------------------------------------
bool SceneSerializer::LoadJson(SceneContext&		 context,
							   const nlohmann::json& root) {
	std::size_t loadedObjectCount = 0;
	std::size_t skippedObjectCount = 0;
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

	// Object生成より先にシーン共通設定を復元する。
	// Collider::ApplyConfigやCollisionManagerは、生成時点から現在シーンのLayer定義を参照できる。
	if(root.is_object() && root.contains("settings")) {
		context.GetSettings().ApplyJson(root.at("settings"));
	} else {
		// settingsを持たないversion 1以前のシーンはDefault設定として扱う。
		context.GetSettings().ResetToDefault();
	}

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
	std::vector<std::pair<Guid, const nlohmann::json*>> loadRecords;
	loadRecords.reserve(jArray.size());

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
			++skippedObjectCount;
			EngineLogger::GetInstance().Add(
				LogLevel::Warning,
				LogCategory::Editor,
				"Scene object type is not registered and was skipped: " + typeName,
				"SceneSerializer");
			continue;
		}
		sp->AdoptPendingSerializableParamCapture(paramOverrides);

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
		loadRecords.emplace_back(guid, &j);
		++loadedObjectCount;
	}

	// 全オブジェクトの登録後、コードの既定値に永続データを一度だけ適用する。
	for(const auto& [guid, data] : loadRecords) {
		auto it = guidMap.find(guid);
		if(it == guidMap.end() || !it->second || !data) continue;
		ApplySceneConfig(*it->second, *data);
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

	for(const auto& j : jArray) {
		Guid ownerGuid = j.value("guid", Guid{});
		if(!ownerGuid.isValid() || !j.contains("boneParentBindings")) continue;

		auto ownerIt = guidMap.find(ownerGuid);
		if(ownerIt == guidMap.end()) continue;
		auto* owner = dynamic_cast<BaseGameObject*>(ownerIt->second.get());
		if(!owner) continue;

		for(const auto& bindingJson : j.at("boneParentBindings")) {
			Guid targetGuid = bindingJson.value("targetGuid", Guid{});
			std::string boneName = bindingJson.value("boneName", std::string{});
			if(!targetGuid.isValid() || boneName.empty()) continue;

			auto targetIt = guidMap.find(targetGuid);
			if(targetIt == guidMap.end() || !targetIt->second) continue;
			owner->SetBoneParent(
				targetIt->second->GetWorldTransform(),
				boneName,
				bindingJson.value("inheritScale", true));
		}
	}
	// 永続データとオブジェクト間の関係が確定してから、実行時状態を初期化する。
	for(const auto& [guid, data] : loadRecords) {
		auto it = guidMap.find(guid);
		if(it == guidMap.end() || !it->second || !data) continue;

		const nlohmann::json* paramOverrides = data->contains("serializableParams")
			? &data->at("serializableParams")
			: nullptr;
		it->second->BeginSerializableParamCapture(paramOverrides);
		it->second->Initialize();
		it->second->EndSerializableParamCapture();
	}

	EngineLogger::GetInstance().Add(
		LogLevel::Trace,
		LogCategory::Editor,
		"Scene JSON loaded. Objects=" + std::to_string(loadedObjectCount) + ", Skipped=" + std::to_string(skippedObjectCount),
		"SceneSerializer");
	return true;
}
