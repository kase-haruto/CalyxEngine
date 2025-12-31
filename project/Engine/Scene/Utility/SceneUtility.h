#pragma once

#include <Engine/Scene/Context/SceneContext.h>

#include <memory>

namespace SceneAPI{
	template<class T, class... Args>
	std::shared_ptr<T> Instantiate(Args&&... args){
		auto ctx = SceneContext::Current();
		assert(ctx && "No active SceneContext!");
		return ctx->Instantiate<T>(std::forward<Args>(args)...);
	}
}

namespace CalyxScene {
	// scene識別id
	using SceneId = uint8_t;
}