#pragma once

#include <vector>

class SceneContext;
class SceneObject;

namespace CalyxEngine {

	class PrefabEditContextUtils {
	public:
		static void MarkEditorUtilityObjects(SceneContext& context);
		static std::vector<SceneObject*> GetSerializableRoots(SceneContext& context);
		static void NormalizeRoots(SceneContext& context);
	};

} // namespace CalyxEngine
