#pragma once

#include <Engine/Application/Effects/EffectAsset.h>
#include <CalyxEngine/Project.h>
#include <Engine/Foundation/Debug/CxAssert.h>
#include <Engine/Application/Effects/EffectPlayer.h>
#include <Data/Engine/Prefab/Serializer/PrefabSerializer.h>
#include <Engine/Foundation/Math/Vector3.h>
#include <Engine/Objects/3D/Actor/SceneObject.h>
#include <Engine/Scene/Context/SceneContext.h>
#include <Engine/Scene/Settings/SceneSettings.h>
#include <Engine/Scene/Fade/BaseSceneTransitionEffect.h>
#include <Engine/Assets/Manager/AssetManager.h>
#include <Engine/PostProcess/Manager/PostEffectManager.h>

#include <filesystem>
#include <memory>
#include <string>
#include <type_traits>
#include <unordered_set>
#include <utility>
#include <vector>

namespace SceneAPI{
	inline void RequestSceneChange(const std::filesystem::path& scenePath) {
		auto* ctx = SceneContext::Current();
		CX_CHECK(ctx && ctx->GetSceneTransitionRequestor(), "Scene transition service is unavailable");
		ctx->GetSceneTransitionRequestor()->RequestSceneChange(scenePath);
	}

	inline void RequestSceneChange(const Guid& sceneAssetGuid) {
		auto* ctx = SceneContext::Current();
		CX_CHECK(ctx && ctx->GetSceneTransitionRequestor(), "Scene transition service is unavailable");
		ctx->GetSceneTransitionRequestor()->RequestSceneChange(sceneAssetGuid);
	}

	inline void RequestSceneChange(const std::filesystem::path& scenePath,
		std::unique_ptr<CalyxEngine::BaseSceneTransitionEffect> effect) {
		auto* ctx = SceneContext::Current();
		CX_CHECK(ctx && ctx->GetSceneTransitionRequestor(), "Scene transition service is unavailable");
		ctx->GetSceneTransitionRequestor()->RequestSceneChange(scenePath, std::move(effect));
	}

	inline void RequestSceneChange(const Guid& sceneAssetGuid,
		std::unique_ptr<CalyxEngine::BaseSceneTransitionEffect> effect) {
		auto* ctx = SceneContext::Current();
		CX_CHECK(ctx && ctx->GetSceneTransitionRequestor(), "Scene transition service is unavailable");
		ctx->GetSceneTransitionRequestor()->RequestSceneChange(sceneAssetGuid, std::move(effect));
	}

	template<class T, class... Args>
	std::shared_ptr<T> Instantiate(Args&&... args){
		auto ctx = SceneContext::Current();
		CX_CHECK(ctx && "No active SceneContext!", "Assertion failed");
		return ctx->Instantiate<T>(std::forward<Args>(args)...);
	}

	inline std::vector<std::shared_ptr<SceneObject>> InstantiatePrefab(
		const std::string& path,
		const CalyxEngine::Vector3& spawnOffset = CalyxEngine::Vector3::Zero(),
		const Guid& prefabAssetGuid = Guid::Empty()) {
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

	template<class T>
	std::vector<std::shared_ptr<T>> InstantiatePrefabRoots(
		const std::string& path,
		const CalyxEngine::Vector3& spawnOffset = CalyxEngine::Vector3::Zero(),
		const Guid& prefabAssetGuid = Guid::Empty()) {
		static_assert(std::is_base_of_v<SceneObject, T>,
					  "T must derive from SceneObject");
		// InstantiatePrefabを呼び出して、ルートオブジェクトを取得する
		// path は現在プロジェクトの AssetRoot/Prefabs 以下からの相対パスで指定する。
	auto objects = InstantiatePrefab(path, spawnOffset, prefabAssetGuid);

		std::unordered_set<SceneObject*> loaded;
		loaded.reserve(objects.size());
		for(const auto& sp : objects) {
			if(sp) loaded.insert(sp.get());
		}

		std::vector<std::shared_ptr<T>> roots;
		for(const auto& sp : objects) {
			if(!sp) continue;
			auto parent = sp->GetParent();
			if(parent && loaded.contains(parent.get())) {
				continue;
			}
			if(auto casted = std::dynamic_pointer_cast<T>(sp)) {
				roots.push_back(casted);
			}
		}

		return roots;
	}

	template<class T>
	std::shared_ptr<T> InstantiatePrefabRoot(
		const std::string& path,
		const CalyxEngine::Vector3& spawnOffset = CalyxEngine::Vector3::Zero(),
		const Guid& prefabAssetGuid = Guid::Empty()) {
		auto roots = InstantiatePrefabRoots<T>(path, spawnOffset, prefabAssetGuid);
		return roots.empty() ? nullptr : roots.front();
	}

	inline const CollisionLayerSettings& GetCollisionLayerSettings() {
		auto ctx = SceneContext::Current();
		CX_CHECK(ctx && "No active SceneContext!", "Assertion failed");
		return ctx->GetSettings().GetCollisionSettings();
	}
}

namespace EffectAPI {
	inline CalyxEngine::EffectPlayer* Player() {
		auto ctx = SceneContext::Current();
		CX_CHECK(ctx && "No active SceneContext!", "Assertion failed");
		return ctx->GetEffectPlayer();
	}

	inline CalyxEngine::EffectHandle Play(const CalyxEngine::EffectAsset& asset,
										  const CalyxEngine::Vector3& position,
										  const CalyxEngine::Quaternion& rotation = CalyxEngine::Quaternion::MakeIdentity(),
										  const CalyxEngine::Vector3& scale = {1.0f, 1.0f, 1.0f}) {
		return Player()->Play(asset, position, rotation, scale);
	}

	inline CalyxEngine::EffectHandle Play(const CalyxEngine::EffectAssetData& data,
										  const CalyxEngine::Vector3& position,
										  const CalyxEngine::Quaternion& rotation = CalyxEngine::Quaternion::MakeIdentity(),
										  const CalyxEngine::Vector3& scale = {1.0f, 1.0f, 1.0f}) {
		return Player()->Play(data, position, rotation, scale);
	}

	inline CalyxEngine::EffectHandle PlayFromName(const std::string& name,
												  const CalyxEngine::Vector3& position,
												  const CalyxEngine::Quaternion& rotation = CalyxEngine::Quaternion::MakeIdentity(),
												  const CalyxEngine::Vector3& scale = {1.0f, 1.0f, 1.0f}) {
		return Player()->PlayFromName(name, position, rotation, scale);
	}

	inline void Stop(CalyxEngine::EffectHandle handle) {
		Player()->Stop(handle);
	}
}

// Game-side audio facade. All functions use the asset's file name (including extension)
// as the stable key, regardless of where the audio asset is stored under Assets.
namespace AudioAPI {
	inline Audio* Manager() {
		auto* manager = CalyxEngine::AssetManager::GetInstance()->GetAudioManager();
		CX_CHECK(manager, "Audio manager is unavailable");
		return manager;
	}

	inline void Load(const std::string& filename) { Manager()->Load(filename); }
	inline void Play(const std::string& filename, bool loop = false, float volume = 1.0f) {
		Manager()->Load(filename);
		Manager()->Play(filename, loop, volume);
	}
	inline void Stop(const std::string& filename) { Manager()->EndAudio(filename); }
	inline void Pause(const std::string& filename) { Manager()->PauseAudio(filename); }
	inline void Restart(const std::string& filename) { Manager()->RestartAudio(filename); }
	inline void SetVolume(const std::string& filename, float volume) { Manager()->SetAudioVolume(filename, volume); }
	inline bool IsPlaying(const std::string& filename) { return Manager()->IsPlayingAudio(filename); }
	inline void Unload(const std::string& filename) { Manager()->UnloadAudio(filename); }
}

// Game-side post-effect facade. The manager itself is also exported for advanced use.
namespace PostEffectAPI {
	inline void Enable(const std::string& name, bool enabled = true) { PostEffectManager::Get()->Enable(name, enabled); }
	inline void Disable(const std::string& name) { PostEffectManager::Get()->Disable(name); }
	inline void Toggle(const std::string& name) { PostEffectManager::Get()->Toggle(name); }
	inline bool IsEnabled(const std::string& name) { return PostEffectManager::Get()->IsEnabled(name); }
	inline void EnableOnly(std::initializer_list<std::string> names) { PostEffectManager::Get()->EnableOnly(names); }
	inline void EnableAll() { PostEffectManager::Get()->EnableAll(); }
	inline void DisableAll() { PostEffectManager::Get()->DisableAll(); }
	inline bool LoadPreset(const std::string& filename) {
		return PostEffectManager::Get()->LoadPreset(
			Calyx::ResolveAssetPath(std::filesystem::path("PostEffects") / filename).generic_string());
	}
	inline void PlayTriggered(const std::string& name) { PostEffectManager::Get()->PlayTriggeredEffect(name); }
	inline void PlayTriggeredAll() { PostEffectManager::Get()->PlayTriggeredEffects(); }
	inline void SetOutlineEnabled(bool enabled) { PostEffectManager::Get()->SetOutlineEnabled(enabled); }
	inline bool IsOutlineEnabled() { return PostEffectManager::Get()->IsOutlineEnabled(); }
}

namespace CalyxEngine {
	// scene識別id
}
