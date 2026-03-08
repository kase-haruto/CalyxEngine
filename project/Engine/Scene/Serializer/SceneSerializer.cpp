#include "SceneSerializer.h"

/* ========================================================================
   include space
   ===================================================================== */
#include <Engine/Application/Effects/FxSystem.h>
#include <Engine/Application/Effects/Particle/Object/ParticleSystemObject.h>
#include <Engine/Foundation/Json/JsonUtils.h>
#include <Engine/Objects/3D/Actor/Library/SceneObjectLibrary.h>
#include <Engine/Objects/3D/Actor/Registry/SceneObjectRegistry.h>
#include <Engine/Scene/Context/SceneContext.h>
#include <Engine/objects/LightObject/DirectionalLight.h>
#include <Engine/objects/LightObject/PointLight.h>
#include <memory>
#include <unordered_map>

using namespace CalyxUtil;

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
			jOne["type"] = sp->GetTypeName();
			jOne["guid"] = sp->GetGuid();
			if(auto parent = sp->GetParent()) {
				jOne["parentGuid"] = parent->GetGuid();
			}

			// ---- 外部設定パスの有無で分岐（SceneObject が保持）----
			const std::string& cfgPath = sp->GetConfigPath();
			if(!cfgPath.empty()) {
				// 個別JSONへ書き出す
				nlohmann::json jCfg;
				cfg->ExtractConfigToJson(jCfg);
				JsonUtils::Save(cfgPath, jCfg);

				// シーンにはパスのみ記録
				jOne["configPath"] = cfgPath;
			} else {
				// 設定を内包
				nlohmann::json jInline;
				cfg->ExtractConfigToJson(jInline);
				// 内包データを jOne にマージ
				for(auto it = jInline.begin(); it != jInline.end(); ++it) {
					jOne[it.key()] = it.value();
				}
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

		auto sp = SceneObjectRegistry::Get().Create(typeName);
		if(!sp) continue;

		if(auto* cfg = dynamic_cast<IConfigurable*>(sp.get())) {
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

		// ライブラリへ登録
		context.GetObjectLibrary()->AddObject(sp);
		sp->Initialize();

		// サブシステムへ橋渡し
		if(auto dir = std::dynamic_pointer_cast<DirectionalLight>(sp)) {
			context.GetLightLibrary()->SetDirectionalLight(dir);
		} else if(auto pt = std::dynamic_pointer_cast<PointLight>(sp)) {
			context.GetLightLibrary()->SetPointLight(pt);
		} else if(auto fx = std::dynamic_pointer_cast<CalyxEffect::ParticleSystemObject>(sp)) {
			context.GetFxSystem()->AddEmitter(fx->GetEmitter(), fx->GetGuid());
		} else if(auto camDbg = std::dynamic_pointer_cast<DebugCamera>(sp)) {
			context.GetCameraMgr()->SetDebugCamera(camDbg);
		} else if(auto camMain = std::dynamic_pointer_cast<Camera3d>(sp)) {
			context.GetCameraMgr()->SetMainCamera(camMain);
		}

		// GUID
		Guid guid	  = j.value("guid", Guid{});
		guidMap[guid] = sp;
	}

	// ---------- 親子リンク ----------
	for(const auto& j : jArray) {
		Guid child	= j.value("guid", Guid{});
		Guid parent = j.value("parentGuid", Guid{});
		if(!child.isValid() || !parent.isValid()) continue;

		auto cIt = guidMap.find(child);
		auto pIt = guidMap.find(parent);
		if(cIt != guidMap.end() && pIt != guidMap.end()) cIt->second->SetParent(pIt->second);
	}
	return true;
}