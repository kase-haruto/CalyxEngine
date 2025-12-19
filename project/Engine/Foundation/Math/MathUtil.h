#pragma once

#include <Engine/Foundation/Math/Matrix4x4.h>
#include <Engine/Foundation/Math/Quaternion.h>
#include <Engine/Foundation/Math/Vector2.h>
#include <Engine/Foundation/Math/Vector3.h>

namespace CalyxMath {
	const float kPi = 3.14159265358979323846f;

	CalyxMath::Matrix4x4 MakeTranslateMatrix(const CalyxMath::Vector3&) noexcept;
	CalyxMath::Matrix4x4 MakeScaleMatrix(const CalyxMath::Vector3&) noexcept;
	CalyxMath::Matrix4x4 MakeRotateXMatrix(float) noexcept;
	CalyxMath::Matrix4x4 MakeRotateYMatrix(float) noexcept;
	CalyxMath::Matrix4x4 MakeRotateZMatrix(float) noexcept;
	CalyxMath::Matrix4x4 MakeAffineMatrix(const CalyxMath::Vector3&, const CalyxMath::Vector3&, const CalyxMath::Vector3&) noexcept;
	CalyxMath::Matrix4x4 MakeAffineMatrix(const CalyxMath::Vector3&, const CalyxMath::Quaternion&, const CalyxMath::Vector3&) noexcept;
	CalyxMath::Matrix4x4 MakeOrthographicMatrix(float l, float t, float r, float b, float n, float f) noexcept;

	CalyxMath::Vector3 TransformNormal(const CalyxMath::Vector3&, const CalyxMath::Matrix4x4&) noexcept;
	CalyxMath::Vector4 MultiplyMatrixVector(const CalyxMath::Matrix4x4&, const CalyxMath::Vector4&) noexcept;

	float Lerp(float a, float b, float t) noexcept;
	float LerpShortAngle(float a, float b, float t) noexcept;
	float ToRadians(float) noexcept;

	CalyxMath::Vector2			WorldToScreen(const CalyxMath::Vector3& worldPos);
	bool			WorldToScreen(const CalyxMath::Vector3& worldPos, CalyxMath::Vector2& outScreenPos);
	CalyxMath::Vector3 ScreenToWorld(const CalyxMath::Vector2& screenPos, float depthZ);

	CalyxMath::Vector3 CatmullRomInterpolation(
		const CalyxMath::Vector3& p0, const CalyxMath::Vector3& p1, const CalyxMath::Vector3& p2, const CalyxMath::Vector3& p3, float t);

	CalyxMath::Vector3 CatmullRomPosition(const std::vector<CalyxMath::Vector3>& points, float t);
} // namespace CalyxMath