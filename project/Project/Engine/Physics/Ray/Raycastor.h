#pragma once

#include <Engine/Objects/3D/Actor/SceneObject.h>
#include <Engine/Objects/3D/Geometory/AABB.h>
#include <Engine/Physics/Ray/RayDetail.h>

#include <optional>
#include <vector>

class Raycastor {
public:
	static std::optional<RaycastHit> Raycast(
		const Ray& ray,
		const std::vector<SceneObject*>& objects,
		float maxDistance = 1000.0f);

	static Ray ConvertMouseToRay(
		const Vector2& mousePos,         // マウス位置（ピクセル）
		const Matrix4x4& viewMatrix,     // カメラのView行列
		const Matrix4x4& projMatrix,     // カメラのProjection行列
		const Vector2& viewportSize      // ビューポートのサイズ（ピクセル）
	);
};

