#pragma once

#include <Engine/Foundation/Math/Vector3.h>
#include <Engine/Foundation/Math/Quaternion.h>
#include <Engine/Objects/Transform/Transform.h>


struct TransformSnapshot {
public:
	static TransformSnapshot FromTransform(const WorldTransform* tf);
	void ApplyToTransform(WorldTransform* tf) const;

	bool Equals(const TransformSnapshot& other, float epsilon = 1e-5f) const;

	CalyxEngine::Vector3 scale;
	CalyxEngine::Quaternion rotate;
	CalyxEngine::Vector3 translate;
};