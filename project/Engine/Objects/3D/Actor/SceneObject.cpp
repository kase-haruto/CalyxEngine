#include "SceneObject.h"
#include "SceneObjectManager.h"
#include <Engine/graphics/Camera/Manager/CameraManager.h>
#include <Engine/Foundation/Utility/Func/MyFunc.h>
#include <Engine/Foundation/Json/JsonUtils.h>
#include <Engine/Objects/ConfigurableObject/IConfigurable.h>
#include <Engine/Objects/3D/Actor/Registry/SceneObjectRegistry.h>
#include <externals/imgui/imgui.h>

static const char* ObjectTypeToString(ObjectType type){
	switch (type){
		case ObjectType::Camera:     return "Camera";
		case ObjectType::Light:      return "Light";
		case ObjectType::GameObject: return "GameObject";
		case ObjectType::ParticleSystem: return "ParticleSystem";
		default:                     return "None";
	}
}

SceneObject::SceneObject(){
	worldTransform_.Initialize();
	id_ = Guid::New();
}	

void SceneObject::ShowGui(){}

AABB SceneObject::FallbackAABBFromTransform() const{
	Vector3 center = worldTransform_.GetWorldPosition();
	Vector3 halfScale = worldTransform_.scale * 0.5f;
	Vector3 min = center - halfScale;
	Vector3 max = center + halfScale;
	return AABB(min, max);
}

std::string SceneObject::GetObjectTypeName() const{
	return ObjectTypeToString(objectType_);
}

void SceneObject::SetName(const std::string& name, ObjectType type){
	name_ = name;
	objectType_ = type;
}

bool SceneObject::HasConfigInterface() const{
	return dynamic_cast< const IConfigurable* >(this) != nullptr;
}

void SceneObject::SetParent(const std::shared_ptr<SceneObject>& newParentSp){
	if (parent_.lock() == newParentSp || newParentSp.get() == this){ return; }

	if (auto oldParent = parent_.lock()){
		auto& siblings = oldParent->children_;
		siblings.erase(std::remove(siblings.begin(), siblings.end(), shared_from_this()),
					   siblings.end());
	}

	if (newParentSp){
		newParentSp->children_.push_back(shared_from_this());

		newParentSp->worldTransform_.Update();

		worldTransform_.parent = &newParentSp->worldTransform_;
	} else{
		worldTransform_.parent = nullptr;
	}

	worldTransform_.Update();

	parent_ = newParentSp;
}

void SceneObject::UpdateWorldTransformRecursive(){
	// 自身のワールド行列を更新
	worldTransform_.Update();

	// 子供たちのワールド行列を再帰的に更新
	for (auto& child : children_){
		child->UpdateWorldTransformRecursive();
	}
}

void SceneObject::AddChild(const std::shared_ptr<SceneObject>& child) {
	if (!child || child.get() == this) return;

	child->SetParent(shared_from_this());
}

REGISTER_SCENE_OBJECT(SceneObject)