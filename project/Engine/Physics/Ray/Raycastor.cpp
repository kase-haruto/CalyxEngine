#include "Raycastor.h"
#include <Engine/Foundation/Utility/Func/MyFunc.h>
#include <cmath>   // fabsf
#include <limits>  // std::numeric_limits

// 極薄AABBや数値誤差で抜けるのを防ぐため、ほんの少し膨らませる
static inline AABB ExpandAabb(const AABB& b, float eps) {
	AABB c = b;
	c.min_ -= CalyxMath::Vector3{ eps, eps, eps };
	c.max_ += CalyxMath::Vector3{ eps, eps, eps };
	return c;
}

static inline int SignBit(float x) { return (x < 0.0f) ? -1 : 1; }

// 0除算ガード・内点OK なレイ vs AABB
bool IntersectRayAABB(const Ray& ray, const AABB& aabbRaw, float& tOut) {
	const AABB aabb = ExpandAabb(aabbRaw, 1e-4f);

	float tmin = 0.0f;
	float tmax = tOut;

	for (int axis = 0; axis < 3; ++axis) {
		const float orig = ray.origin[axis];
		const float dir = ray.direction[axis];
		const float bmin = aabb.min_[axis];
		const float bmax = aabb.max_[axis];

		// 平行：スラブ外なら交差なし
		if (std::fabs(dir) < 1e-8f) {
			if (orig < bmin || orig > bmax) return false;
			continue; // 内側なのでこの軸は制約しない
		}

		const float invD = 1.0f / dir;
		float t0 = (bmin - orig) * invD;
		float t1 = (bmax - orig) * invD;
		if (t0 > t1) std::swap(t0, t1);

		tmin = (std::max)(tmin, t0);
		tmax = (std::min)(tmax, t1);
		if (tmax < tmin) return false;
	}

	tOut = tmin;  // 内点なら 0 に近い値が返る
	return true;
}

// t に対応する当たり面の法線をざっくり算出（スラブ判定）
static inline CalyxMath::Vector3 ComputeAabbHitNormal(const Ray& ray, const AABB& aabb, float t) {
	const CalyxMath::Vector3 p = ray.origin + ray.direction * t;
	const float eps = 1e-3f; // 近さ判定

	// どの面から入ったか（近い面を優先）
	if (std::fabs(p.x - aabb.min_.x) < eps) return CalyxMath::Vector3{ -1, 0, 0 };
	if (std::fabs(p.x - aabb.max_.x) < eps) return CalyxMath::Vector3{ 1, 0, 0 };
	if (std::fabs(p.y - aabb.min_.y) < eps) return CalyxMath::Vector3{ 0,-1, 0 };
	if (std::fabs(p.y - aabb.max_.y) < eps) return CalyxMath::Vector3{ 0, 1, 0 };
	if (std::fabs(p.z - aabb.min_.z) < eps) return CalyxMath::Vector3{ 0, 0,-1 };
	if (std::fabs(p.z - aabb.max_.z) < eps) return CalyxMath::Vector3{ 0, 0, 1 };

	// うまく取れない場合はレイの最も支配的な軸の反対向きにしておく
	const float ax = std::fabs(ray.direction.x);
	const float ay = std::fabs(ray.direction.y);
	const float az = std::fabs(ray.direction.z);
	if (ax >= ay && ax >= az) return CalyxMath::Vector3{ (float)-SignBit(ray.direction.x), 0, 0 };
	if (ay >= ax && ay >= az) return CalyxMath::Vector3{ 0, (float)-SignBit(ray.direction.y), 0 };
	return CalyxMath::Vector3{ 0, 0, (float)-SignBit(ray.direction.z) };
}

std::optional<RaycastHit>
Raycastor::Raycast(const Ray& ray,
				   const std::vector<SceneObject*>& objects,
				   float maxDistance) {
	std::optional<RaycastHit> closestHit;

	for (auto* obj : objects) {
		if (!obj) continue;
		if (!obj->IsEnableRaycast()) continue;

		// ワールドAABBを少し膨らませてヒット漏れを防ぐ
		AABB box = ExpandAabb(obj->GetWorldAABB(), 1e-4f);

		float t = maxDistance;
		if (IntersectRayAABB(ray, box, t) && t <= maxDistance) {
			// 内点（t が 0 に極めて近い）でスタートしているなら、表面までわずかに押し出す
			if (t < 0.0f) t = 0.0f;

			RaycastHit hit;
			hit.distance = t;
			hit.point = ray.origin + ray.direction * t;
			hit.normal = ComputeAabbHitNormal(ray, box, t);
			hit.hitObject = obj;

			closestHit = hit;
			maxDistance = t; // さらに近いものだけを受け付ける
		}
	}
	return closestHit;
}

Ray Raycastor::ConvertMouseToRay(const CalyxMath::Vector2& mousePos,
								 const CalyxMath::Matrix4x4& view,
								 const CalyxMath::Matrix4x4& proj,
								 const CalyxMath::Vector2& viewportSize) {
	// 1. NDC
	const float ndcX = (2.0f * mousePos.x) / viewportSize.x - 1.0f;
	const float ndcY = 1.0f - (2.0f * mousePos.y) / viewportSize.y; // DirectX: Y反転

	// 2. クリップ空間の near / far
	const CalyxMath::Vector4 nearPoint(ndcX, ndcY, 0.0f, 1.0f);
	const CalyxMath::Vector4 farPoint(ndcX, ndcY, 1.0f, 1.0f);

	// 3. View 空間へ
	const CalyxMath::Matrix4x4 invProj = CalyxMath::Matrix4x4::Inverse(proj);
	CalyxMath::Vector4 nearView = invProj * nearPoint;
	CalyxMath::Vector4 farView = invProj * farPoint;

	// 同次座標正規化
	nearView /= nearView.w;
	farView /= farView.w;

	// 4. World 空間へ
	const CalyxMath::Matrix4x4 invView = CalyxMath::Matrix4x4::Inverse(view);
	const CalyxMath::Vector4 nearWorld = invView * nearView;
	const CalyxMath::Vector4 farWorld = invView * farView;

	// 5. レイ作成（正規化）
	CalyxMath::Vector3 origin = nearWorld.xyz();
	CalyxMath::Vector3 direction = (farWorld.xyz() - origin).Normalize();

	return Ray{ origin, direction };
}
