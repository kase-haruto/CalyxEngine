#include "Camera3d.h"
/* ========================================================================
/* include space
/* ===================================================================== */
#include <Engine/Graphics/Context/GraphicsGroup.h>

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

void Camera3d::DrawFrustum(){
	Matrix4x4 invViewProj = Matrix4x4::Inverse(viewProjectionMatrix_);

	// DirectXのNDC空間：z = 0 (near), z = 1 (far)
	Vector3 ndcCorners[8] = {
		{-1, -1, 0}, {1, -1, 0}, {1, 1, 0}, {-1, 1, 0},     // near plane
		{-1, -1, 1}, {1, -1, 1}, {1, 1, 1}, {-1, 1, 1}      // far plane
	};

	Vector3 worldCorners[8];
	for (int i = 0; i < 8; ++i){
		const Vector3& ndc = ndcCorners[i];
		Vector4 clip(ndc.x, ndc.y, ndc.z, 1.0f); // ← 修正ポイント
		Vector4 world = Vector4::Transform(clip, invViewProj);

		// 透視除算
		if (world.w != 0.0f){
			worldCorners[i] = Vector3(world.x / world.w, world.y / world.w, world.z / world.w);
		}
	}

	Vector4 color = {1, 1, 0, 1}; // 黄色

	// near plane
	PrimitiveDrawer::GetInstance()->DrawLine3d(worldCorners[0], worldCorners[1], color);
	PrimitiveDrawer::GetInstance()->DrawLine3d(worldCorners[1], worldCorners[2], color);
	PrimitiveDrawer::GetInstance()->DrawLine3d(worldCorners[2], worldCorners[3], color);
	PrimitiveDrawer::GetInstance()->DrawLine3d(worldCorners[3], worldCorners[0], color);

	// far plane
	PrimitiveDrawer::GetInstance()->DrawLine3d(worldCorners[4], worldCorners[5], color);
	PrimitiveDrawer::GetInstance()->DrawLine3d(worldCorners[5], worldCorners[6], color);
	PrimitiveDrawer::GetInstance()->DrawLine3d(worldCorners[6], worldCorners[7], color);
	PrimitiveDrawer::GetInstance()->DrawLine3d(worldCorners[7], worldCorners[4], color);

	// connect near and far planes
	PrimitiveDrawer::GetInstance()->DrawLine3d(worldCorners[0], worldCorners[4], color);
	PrimitiveDrawer::GetInstance()->DrawLine3d(worldCorners[1], worldCorners[5], color);
	PrimitiveDrawer::GetInstance()->DrawLine3d(worldCorners[2], worldCorners[6], color);
	PrimitiveDrawer::GetInstance()->DrawLine3d(worldCorners[3], worldCorners[7], color);
}


void Camera3d::Update(){
    BaseCamera::Update();

	DrawFrustum();
}

void Camera3d::ShowGui(){
	//名前の表示
	SceneObject::ShowGui();

	// アクティブかどうか
	BaseCamera::ShowGui();
}

