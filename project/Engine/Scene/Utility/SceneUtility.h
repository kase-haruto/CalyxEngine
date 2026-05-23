#pragma once

#include <Engine/Application/Effects/EffectAsset.h>
#include <Engine/Foundation/Debug/CxAssert.h>
#include <Engine/Application/Effects/EffectPlayer.h>
#include <Engine/Scene/Context/SceneContext.h>

#include <filesystem>
#include <memory>

namespace SceneAPI{
	template<class T, class... Args>
	std::shared_ptr<T> Instantiate(Args&&... args){
		auto ctx = SceneContext::Current();
		CX_CHECK(ctx && "No active SceneContext!", "Assertion failed");
		return ctx->Instantiate<T>(std::forward<Args>(args)...);
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