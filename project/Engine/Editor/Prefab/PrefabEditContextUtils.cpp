#include "PrefabEditContextUtils.h"

#include <Engine/Foundation/Math/Vector3.h>
#include <Engine/Objects/3D/Actor/Library/SceneObjectLibrary.h>
#include <Engine/Objects/3D/Actor/SceneObject.h>
#include <Engine/Scene/Context/SceneContext.h>

#include <string_view>

namespace CalyxEngine {

	void PrefabEditContextUtils::MarkEditorUtilityObjects(SceneContext& context) {
		auto* library = context.GetObjectLibrary();
		if(!library) return;

		// Preview用Camera/LightをTransient化し、Prefab Assetへの混入とRaycast選択を防ぐ。
		for(auto* object : library->GetAllObjectsRaw()) {
			if(!object) continue;

			const std::string_view className = object->GetObjectClassName();
			const std::string& name = object->GetName();
			if(className == "Camera3d" || className == "DebugCamera" || className == "BaseCamera" ||
			   name == "MainCamera" || name == "DebugCamera" ||
			   name == "PrefabPreviewDirectionalLight" || name == "PrefabPreviewPointLight") {
				object->SetTransient(true);
				object->SetEnableRaycast(false);
			}
		}
	}

	std::vector<SceneObject*> PrefabEditContextUtils::GetSerializableRoots(SceneContext& context) {
		std::vector<SceneObject*> roots;
		auto* library = context.GetObjectLibrary();
		if(!library) return roots;

		// 保存可能かつ親を持たないObjectだけをSerializerのRootとして収集する。
		for(auto* object : library->GetAllObjectsRaw()) {
			if(!object || !object->IsSerializable()) continue;
			if(object->GetParent()) continue;
			roots.push_back(object);
		}
		return roots;
	}

	void PrefabEditContextUtils::NormalizeRoots(SceneContext& context) {
		// 保存前にUtility属性を再確認し、Prefab RootをAsset原点へ揃える。
		MarkEditorUtilityObjects(context);
		for(auto* root : GetSerializableRoots(context)) {
			if(!root) continue;
			auto& transform = root->GetWorldTransform();
			// Scene上の配置はInstance側が持つため、Prefab AssetにはRootの移動量を保存しない。
			transform.translation = CalyxEngine::Vector3::Zero();
			transform.Update();
		}
	}

} // namespace CalyxEngine
