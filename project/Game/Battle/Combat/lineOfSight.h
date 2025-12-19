#pragma once
#include <functional>
#include <vector>

#include <Engine/Foundation/Math/Vector3.h>
#include <Engine/Physics/Ray/RayDetail.h>
#include <Engine/Physics/Ray/Raycastor.h>

namespace CalyxUtil {
	bool RaycastSegment(const CalyxMath::Vector3& from, const CalyxMath::Vector3& to,
						const std::vector<SceneObject*>& objects,
						RaycastHit& outHit, float startOffset = 0.05f);

	bool HasLineOfSight(const CalyxMath::Vector3& from, const CalyxMath::Vector3& to,
						const std::vector<SceneObject*>&  objects,
						const std::function<bool(void*)>& isPlayer,
						float							  startOffset = 0.05f);
}

