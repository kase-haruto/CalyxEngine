#pragma once

#include <vector>

class SceneContext;
class SceneObject;

namespace CalyxEngine {

	/**
	 * @brief PrefabEditContextUtilsの機能を提供するクラスです。
	 */
	class PrefabEditContextUtils {
	public:
		static void MarkEditorUtilityObjects(SceneContext& context);
		static std::vector<SceneObject*> GetSerializableRoots(SceneContext& context);
		static void NormalizeRoots(SceneContext& context);
	};

} // namespace CalyxEngine
