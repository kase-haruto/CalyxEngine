#include "SceneUtility.h"

////////////////////////////////////////////////////////////////////////
//		シーン変更をRequest
////////////////////////////////////////////////////////////////////////
void SceneAPI::RequestSceneChange(const std::filesystem::path& scenePath) {
	auto* ctx = SceneContext::Current();
	CX_CHECK(ctx && ctx->GetSceneTransitionRequestor(), "Scene transition service is unavailable");
	ctx->GetSceneTransitionRequestor()->RequestSceneChange(scenePath);
}

void SceneAPI::RequestSceneChange(const Guid& sceneAssetGuid) {
	auto* ctx = SceneContext::Current();
	CX_CHECK(ctx && ctx->GetSceneTransitionRequestor(), "Scene transition service is unavailable");
	ctx->GetSceneTransitionRequestor()->RequestSceneChange(sceneAssetGuid);
}

void SceneAPI::RequestSceneChange(const std::filesystem::path& scenePath, std::unique_ptr<CalyxEngine::BaseSceneTransitionEffect> effect) {
	auto* ctx = SceneContext::Current();
	CX_CHECK(ctx && ctx->GetSceneTransitionRequestor(), "Scene transition service is unavailable");
	ctx->GetSceneTransitionRequestor()->RequestSceneChange(scenePath, std::move(effect));
}

void SceneAPI::RequestSceneChange(const Guid& sceneAssetGuid, std::unique_ptr<CalyxEngine::BaseSceneTransitionEffect> effect) {
	auto* ctx = SceneContext::Current();
	CX_CHECK(ctx && ctx->GetSceneTransitionRequestor(), "Scene transition service is unavailable");
	ctx->GetSceneTransitionRequestor()->RequestSceneChange(sceneAssetGuid, std::move(effect));
}

bool SceneAPI::RemoveObject(const std::shared_ptr<SceneObject>& object) {
	auto* ctx = SceneContext::Current();
	if(!ctx || !object || !ctx->GetObjectLibrary() ||
	   !ctx->GetObjectLibrary()->Contains(object)) {
		return false;
	}

	ctx->RemoveObject(object);
	return true;
}

bool SceneAPI::RemoveObject(SceneObject* object) {
	auto* ctx = SceneContext::Current();
	if(!ctx || !object) {
		return false;
	}

	return RemoveObject(ctx->FindSharedObject(object));
}

/////////////////////////////////////////////////////////////////////////////////////////
//  prefabをインスタンス化
/////////////////////////////////////////////////////////////////////////////////////////
std::vector<std::shared_ptr<SceneObject>> SceneAPI::InstantiatePrefab(const std::string& path, const CalyxEngine::Vector3& spawnOffset, const Guid& prefabAssetGuid) {
	auto ctx = SceneContext::Current();
	CX_CHECK(ctx && "No active SceneContext!", "Assertion failed");
	std::string fullPath = Calyx::ResolveAssetPath(std::filesystem::path("Prefabs") / path).generic_string();

	auto objects = PrefabSerializer::Load(
		fullPath,
		PrefabSerializer::LoadOptions{false, prefabAssetGuid});

	std::unordered_set<SceneObject*> loaded;
	loaded.reserve(objects.size());
	for(const auto& sp : objects) {
		if(sp) loaded.insert(sp.get());
	}

	for(const auto& sp : objects) {
		if(!sp) continue;

		if(prefabAssetGuid.isValid() && !sp->GetPrefabAssetGuid().isValid()) {
			sp->SetPrefabLink(prefabAssetGuid, sp->GetGuid());
		}

		auto parent = sp->GetParent();
		if(!parent || !loaded.contains(parent.get())) {
			sp->GetWorldTransform().translation =
				sp->GetWorldTransform().translation + spawnOffset;
		}

		ctx->AddObject(sp);
	}

	return objects;
}

/////////////////////////////////////////////////////////////////////////////////////////
//		CollisionLayerの設定を取得する
/////////////////////////////////////////////////////////////////////////////////////////
const CollisionLayerSettings& SceneAPI::GetCollisionLayerSettings() {
	auto ctx = SceneContext::Current();
	CX_CHECK(ctx && "No active SceneContext!", "Assertion failed");
	return ctx->GetSettings().GetCollisionSettings();
}
