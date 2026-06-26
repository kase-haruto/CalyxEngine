#pragma once

#include <Engine/Application/Effects/EffectAsset.h>
#include <Engine/Foundation/Debug/CxAssert.h>
#include <Engine/Application/Effects/EffectPlayer.h>
#include <Data/Engine/Prefab/Serializer/PrefabSerializer.h>
#include <Engine/Foundation/Math/Vector3.h>
#include <Engine/Objects/3D/Actor/SceneObject.h>
#include <Engine/Scene/Context/SceneContext.h>

#include <filesystem>
#include <memory>
#include <string>
#include <type_traits>
#include <unordered_set>
#include <utility>
#include <vector>

namespace SceneAPI{
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
		std::string fullPath = "Resources/Assets/Prefabs/"+path;

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
		// pathはResources/Assets/Prefabs/以下のパスで指定する
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

namespace CalyxEngine {
	// scene識別id
	using SceneId = uint8_t;
}