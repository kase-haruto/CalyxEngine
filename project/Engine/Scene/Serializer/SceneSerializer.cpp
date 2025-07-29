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
bool SceneSerializer::LoadJson(SceneContext& context,
                               const nlohmann::json& root) {
    /* ---------- ① JSON 配列取得（変更なし） ---------- */
    nlohmann::json jArray;
    if (root.is_array())        jArray = root;                       // 旧形式
    else                        jArray = root.value("objects", nlohmann::json::array());

    if (root.contains("sceneName"))
        context.SetSceneName(root.value("sceneName", "scene"));

    /* ---------- ② 既存クリア ---------- */
    context.Clear();

    // ★ Light & Camera を一旦無効化
    if (auto* ll = context.GetLightLibrary()) {
        ll->SetDirectionalLight({});
        ll->SetPointLight({});
    }
    if (auto* cm = context.GetCameraMgr()) {
        cm->SetMainCamera({});   // 関数名は後述
        cm->SetDebugCamera({});
    }

    std::unordered_map<Guid, std::shared_ptr<SceneObject>> guidMap;

    /* ---------- ③ 1st pass 生成 & サブシステム登録 ---------- */
    for (const auto& j : jArray) {
        std::string typeName = j.value("type", "");
        if (typeName.empty()) continue;

        auto sp = SceneObjectRegistry::Get().Create(typeName);
        if (!sp) continue;

        // IConfigurable
        if (auto* cfg = dynamic_cast<IConfigurable*>(sp.get()))
            cfg->ApplyConfigFromJson(j);

        // ライブラリへ
        context.GetObjectLibrary()->AddObject(sp);

        // ── サブシステム判定 ───────────────────────────
        if (auto dir = std::dynamic_pointer_cast<DirectionalLight>(sp)) {
            context.GetLightLibrary()->SetDirectionalLight(dir);

        } else if (auto pt = std::dynamic_pointer_cast<PointLight>(sp)) {
            context.GetLightLibrary()->SetPointLight(pt);

        } else if (auto fx = std::dynamic_pointer_cast<ParticleSystemObject>(sp)) {
            context.GetFxSystem()->AddEmitter(fx);

        } else if (auto camMain = std::dynamic_pointer_cast<Camera3d>(sp)) {
            context.GetCameraMgr()->SetMainCamera(camMain);

        } else if (auto camDbg = std::dynamic_pointer_cast<DebugCamera>(sp)) {
            context.GetCameraMgr()->SetDebugCamera(camDbg);
        }

        // GUID
        Guid guid = j.value("guid", Guid{});
        guidMap[guid] = sp;
    }

    /* ---------- ④ 2nd pass 親子リンク ---------- */
    for (const auto& j : jArray) {
        Guid child = j.value("guid", Guid{});
        Guid parent = j.value("parentGuid", Guid{});
        if (!child.isValid() || !parent.isValid()) continue;

        auto cIt = guidMap.find(child);
        auto pIt = guidMap.find(parent);
        if (cIt != guidMap.end() && pIt != guidMap.end())
            cIt->second->SetParent(pIt->second);
    }
    return true;
}
