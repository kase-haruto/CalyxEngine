#pragma once

#include <Engine/Foundation/Math/Matrix4x4.h>
#include <Engine/Foundation/Math/Quaternion.h>
#include <Engine/Foundation/Math/Vector2.h>
#include <Engine/Foundation/Math/Vector3.h>

namespace CxMath {
	const float kPi = 3.14159265358979323846f;

	CxMath::Matrix4x4 MakeTranslateMatrix(const CxMath::Vector3&) noexcept;
	CxMath::Matrix4x4 MakeScaleMatrix(const CxMath::Vector3&) noexcept;
	CxMath::Matrix4x4 MakeRotateXMatrix(float) noexcept;
	CxMath::Matrix4x4 MakeRotateYMatrix(float) noexcept;
	CxMath::Matrix4x4 MakeRotateZMatrix(float) noexcept;
	CxMath::Matrix4x4 MakeAffineMatrix(const CxMath::Vector3&, const CxMath::Vector3&, const CxMath::Vector3&) noexcept;
	CxMath::Matrix4x4 MakeAffineMatrix(const CxMath::Vector3&, const CxMath::Quaternion&, const CxMath::Vector3&) noexcept;
	CxMath::Matrix4x4 MakeOrthographicMatrix(float l, float t, float r, float b, float n, float f) noexcept;

	CxMath::Vector3 TransformNormal(const CxMath::Vector3&, const CxMath::Matrix4x4&) noexcept;
	CxMath::Vector4 MultiplyMatrixVector(const CxMath::Matrix4x4&, const CxMath::Vector4&) noexcept;

	float Lerp(float a, float b, float t) noexcept;
	float LerpShortAngle(float a, float b, float t) noexcept;
	float ToRadians(float) noexcept;

	CxMath::Vector2			WorldToScreen(const CxMath::Vector3& worldPos);
	bool			WorldToScreen(const CxMath::Vector3& worldPos, CxMath::Vector2& outScreenPos);
	CxMath::Vector3 ScreenToWorld(const CxMath::Vector2& screenPos, float depthZ);

	CxMath::Vector3 CatmullRomInterpolation(
		const CxMath::Vector3& p0, const CxMath::Vector3& p1, const CxMath::Vector3& p2, const CxMath::Vector3& p3, float t);

	CxMath::Vector3 CatmullRomPosition(const std::vector<CxMath::Vector3>& points, float t);
} // namespace CxMath