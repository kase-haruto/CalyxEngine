#pragma once

#include <Engine/Foundation/Export/CalyxAPI.h>

#include <Engine/Foundation/Math/Matrix4x4.h>
#include <Engine/Foundation/Math/Quaternion.h>
#include <Engine/Foundation/Math/Vector2.h>
#include <Engine/Foundation/Math/Vector3.h>

namespace  CalyxEngine{
	const float kPi = 3.14159265358979323846f;
	const float kTwoPi = 2.0f * kPi;

	CALYX_API Matrix4x4 MakeTranslateMatrix(const Vector3&) noexcept;
	CALYX_API Matrix4x4 MakeScaleMatrix(const Vector3&) noexcept;
	CALYX_API Matrix4x4 MakeRotateXMatrix(float) noexcept;
	CALYX_API Matrix4x4 MakeRotateYMatrix(float) noexcept;
	CALYX_API Matrix4x4 MakeRotateZMatrix(float) noexcept;
	CALYX_API Matrix4x4 MakeAffineMatrix(const Vector3&, const Vector3&, const Vector3&) noexcept;
	CALYX_API Matrix4x4 MakeAffineMatrix(const Vector3&, const Quaternion&, const Vector3&) noexcept;
	CALYX_API Matrix4x4 MakeOrthographicMatrix(float l, float t, float r, float b, float n, float f) noexcept;

	CALYX_API Vector3 TransformNormal(const Vector3&, const Matrix4x4&) noexcept;
	CALYX_API Vector4 MultiplyMatrixVector(const Matrix4x4&, const Vector4&) noexcept;

	CALYX_API float Lerp(float a, float b, float t) noexcept;
	CALYX_API float LerpShortAngle(float a, float b, float t) noexcept;
	CALYX_API float ToRadians(float) noexcept;

	CALYX_API Vector2			WorldToScreen(const Vector3& worldPos);
	CALYX_API bool			WorldToScreen(const Vector3& worldPos, Vector2& outScreenPos);
	CALYX_API Vector3 ScreenToWorld(const Vector2& screenPos, float depthZ);

	CALYX_API Vector3 CatmullRomInterpolation(
		const Vector3& p0, const Vector3& p1, const Vector3& p2, const Vector3& p3, float t);

	CALYX_API Vector3 CatmullRomPosition(const std::vector<Vector3>& points, float t);
} // namespace 