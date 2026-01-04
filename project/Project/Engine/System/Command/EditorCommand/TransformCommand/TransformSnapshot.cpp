#include "TransformSnapshot.h"

TransformSnapshot TransformSnapshot::FromTransform(const WorldTransform* tf) {
	return { tf->scale, tf->rotation, tf->translation };
}

void TransformSnapshot::ApplyToTransform(WorldTransform* tf) const {
	tf->scale = scale;
	tf->rotation = rotate;
	tf->translation = translate;
}

bool TransformSnapshot::Equals(const TransformSnapshot& other, float epsilon) const {
	return (translate - other.translate).LengthSquared() < epsilon &&
		(scale - other.scale).LengthSquared() < epsilon &&
		Quaternion::Dot(rotate, other.rotate) > 1.0f - epsilon;
}