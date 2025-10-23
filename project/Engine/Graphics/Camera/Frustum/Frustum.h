#pragma once

// engine
#include <Engine/Foundation/Math/Vector3.h>
#include <Engine/Foundation/Math/Matrix4x4.h>
#include <Engine/Foundation/Math/Vector4.h>
// c+
#include <array>

struct FrustumPlane{
	Vector3 normal;
	float distance;

	float GetSignedDistanceToPoint(const Vector3& point) const{
		return Vector3::Dot(normal, point) + distance;
	}
};

class Frustum{
public:
	Frustum() = default;

	void ExtractFromMatrix(const Matrix4x4& viewProj);
	bool IsAABBInside(const Vector3& min, const Vector3& max) const;
	void Draw(const Vector4& color = Vector4(1, 1, 0, 1), float farPlaneRatio = 0.005f) const;
	void CalculateCorners(Vector3 outCorners[8]) const;

private:
	FrustumPlane NormalizePlane(const Vector4& p);

private:
	std::array<FrustumPlane, 6> planes_;
	Matrix4x4 viewProjection_;

	
};
