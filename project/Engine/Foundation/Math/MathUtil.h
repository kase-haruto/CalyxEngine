#pragma once

#include <Engine/Foundation/Math/Matrix4x4.h>
#include <Engine/Foundation/Math/Quaternion.h>
#include <Engine/Foundation/Math/Vector2.h>
#include <Engine/Foundation/Math/Vector3.h>

namespace  CalyxMath{
	const float kPi = 3.14159265358979323846f;
	const float kTwoPi = 2.0f * kPi;

	Matrix4x4 MakeTranslateMatrix(const Vector3&) noexcept;
	Matrix4x4 MakeScaleMatrix(const Vector3&) noexcept;
	Matrix4x4 MakeRotateXMatrix(float) noexcept;
	Matrix4x4 MakeRotateYMatrix(float) noexcept;
	Matrix4x4 MakeRotateZMatrix(float) noexcept;
	Matrix4x4 MakeAffineMatrix(const Vector3&, const Vector3&, const Vector3&) noexcept;
	Matrix4x4 MakeAffineMatrix(const Vector3&, const Quaternion&, const Vector3&) noexcept;
	Matrix4x4 MakeOrthographicMatrix(float l, float t, float r, float b, float n, float f) noexcept;

	Vector3 TransformNormal(const Vector3&, const Matrix4x4&) noexcept;
	Vector4 MultiplyMatrixVector(const Matrix4x4&, const Vector4&) noexcept;

	float Lerp(float a, float b, float t) noexcept;
	float LerpShortAngle(float a, float b, float t) noexcept;
	float ToRadians(float) noexcept;

	Vector2			WorldToScreen(const Vector3& worldPos);
	bool			WorldToScreen(const Vector3& worldPos, Vector2& outScreenPos);
	Vector3 ScreenToWorld(const Vector2& screenPos, float depthZ);

	Vector3 CatmullRomInterpolation(
		const Vector3& p0, const Vector3& p1, const Vector3& p2, const Vector3& p3, float t);

	Vector3 CatmullRomPosition(const std::vector<Vector3>& points, float t);
} // namespace 