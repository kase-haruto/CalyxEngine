#include "Camera3d.h"
/* ========================================================================
/* include space
/* ===================================================================== */
#include <Engine/Graphics/Context/GraphicsGroup.h>
#include <Engine/Objects/3D/Geometory/AABB.h>

// lib
#include <Engine/Foundation/Utility/Func/MyFunc.h>

// c++
#include <externals/imgui/imgui.h>
#include <cmath>

Camera3d::Camera3d()
    : BaseCamera(){
	BaseCamera::SetName("MainCamera");
	transform_.translate = {0.0f, 2.0f, -10.0f};
}

void Camera3d::Initialize() {
	transform_.translate = { 0.0f, 2.0f, -10.0f };
}

void Camera3d::Update(){
    BaseCamera::Update();

	frustum_.ExtractFromMatrix(viewProjectionMatrix_);
	frustum_.Draw();
}

void Camera3d::ShowGui(){
	//名前の表示
	SceneObject::ShowGui();

	// アクティブかどうか
	BaseCamera::ShowGui();
}

bool Camera3d::IsVisible(const AABB& aabb) const{
	return frustum_.IsAABBInside(aabb.min_, aabb.max_);
}

