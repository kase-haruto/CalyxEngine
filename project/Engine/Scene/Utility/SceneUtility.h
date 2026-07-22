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
	CALYX_API void RequestSceneChange(const std::filesystem::path& scenePath);
	CALYX_API void RequestSceneChange(const Guid& sceneAssetGuid);
	CALYX_API void RequestSceneChange(
		const std::filesystem::path& scenePath,
		std::unique_ptr<CalyxEngine::BaseSceneTransitionEffect> effect);
	CALYX_API void RequestSceneChange(
		const Guid& sceneAssetGuid,
		std::unique_ptr<CalyxEngine::BaseSceneTransitionEffect> effect);

	template<class T, class... Args>
	std::shared_ptr<T> Instantiate(Args&&... args){
		auto ctx = SceneContext::Current();
		CX_CHECK(ctx && "No active SceneContext!", "Assertion failed");
		return ctx->Instantiate<T>(std::forward<Args>(args)...);
	}

	CALYX_API std::vector<std::shared_ptr<SceneObject>> InstantiatePrefab(
		const std::string& path,
		const CalyxEngine::Vector3& spawnOffset = CalyxEngine::Vector3::Zero(),
		const Guid& prefabAssetGuid = Guid::Empty());

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

	CALYX_API const CollisionLayerSettings& GetCollisionLayerSettings();
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

// ゲーム側のオーディオ用API。Assets以下の保存場所にかかわらず、
// 拡張子を含むファイル名を共通のキーとして使用する。
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

// ゲーム側のポストエフェクト用API。高度な用途向けにManager本体も公開している。
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
	inline bool SetVignetteColor(const CalyxEngine::Vector3& color) {
		auto* pass = PostEffectManager::Get()->GetPass("Vignette");
		if(!pass) return false;
		const bool r = pass->SetFloatParameter("color.r", color.x);
		const bool g = pass->SetFloatParameter("color.g", color.y);
		const bool b = pass->SetFloatParameter("color.b", color.z);
		return r && g && b;
	}
	inline bool SetVignetteColor(float r, float g, float b) {
		return SetVignetteColor(CalyxEngine::Vector3{r, g, b});
	}
	inline void SetOutlineEnabled(bool enabled) { PostEffectManager::Get()->SetOutlineEnabled(enabled); }
	inline bool IsOutlineEnabled() { return PostEffectManager::Get()->IsOutlineEnabled(); }
}

namespace CalyxEngine {
	// scene識別id
}
