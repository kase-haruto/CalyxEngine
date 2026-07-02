#include "SceneObjectReference.h"

#include <Engine/Objects/3D/Actor/Library/SceneObjectLibrary.h>
#include <Engine/Scene/Context/SceneContext.h>

namespace CalyxEngine {

	const ISceneObjectResolver* GetCurrentSceneObjectResolver() {
		// ゲーム停止中やシーン切り替え中はCurrentが存在しない可能性を許容する。
		auto* context = SceneContext::Current();
		if(!context) return nullptr;

		// SceneObjectLibraryはISceneObjectResolverを実装しているため、その抽象契約だけを返す。
		return context->GetObjectLibrary();
	}

} // namespace CalyxEngine
