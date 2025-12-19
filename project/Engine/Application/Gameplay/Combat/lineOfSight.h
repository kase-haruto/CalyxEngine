#pragma once
#include <functional>
#include <vector>

#include <Engine/Foundation/Math/Vector3.h>
#include <Engine/Physics/Ray/RayDetail.h>
#include <Engine/Physics/Ray/Raycastor.h>

inline bool RaycastSegment(const CalyxMath::Vector3& from, const CalyxMath::Vector3& to,
						   const std::vector<SceneObject*>& objects,
						   RaycastHit& outHit, float startOffset = 0.05f) {
	CalyxMath::Vector3 dir = to - from;
	float   dist = dir.Length();
	if (dist <= 1e-4f) return false; //区間長ゼロ

	dir /= dist;

	// 自己ヒット回避のため、少しだけ前にオフセットして撃つ
	Ray ray{ from + dir * startOffset, dir };

	const float maxSeg = (std::max)(0.0f, dist - startOffset);
	auto hitOpt = Raycastor::Raycast(ray, objects, maxSeg);
	if (!hitOpt) return false;

	outHit = *hitOpt; //近いヒット
	return true;
}

inline bool HasLineOfSight(const CalyxMath::Vector3& from, const CalyxMath::Vector3& to,
						   const std::vector<SceneObject*>& objects,
						   const std::function<bool(void*)>& isPlayer,
						   float startOffset = 0.05f) {
	RaycastHit hit{};
	if (!RaycastSegment(from, to, objects, hit, startOffset)) {
		//遮蔽物なし
		return true;
	}
	return isPlayer ? isPlayer(hit.hitObject) : true;
}
