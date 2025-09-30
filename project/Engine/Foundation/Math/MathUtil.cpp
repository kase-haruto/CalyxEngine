#include "MathUtil.h"

// math
#include <Engine/Foundation/Math/Vector2.h>
#include <Engine/Foundation/Math/Vector3.h>
#include <Engine/Foundation/Math/Matrix4x4.h>
#include <Engine/Foundation/Math/Quaternion.h>

// engine
#include <Engine/Graphics/Camera/Manager/CameraManager.h>

// env
#include <Engine/Application/System/Enviroment.h>

#include <cmath>

namespace Cx::Math{
Matrix4x4 MakeTranslateMatrix(const Vector3& translate)noexcept{
	Matrix4x4 result = {
		1, 0, 0, 0,
		0, 1, 0, 0,
		0, 0, 1, 0,
		translate.x, translate.y, translate.z, 1
	};
	return result;
}


Matrix4x4 MakeScaleMatrix(const Vector3& scale)noexcept{
	Matrix4x4 result = {
		scale.x, 0, 0, 0,
		0, scale.y, 0, 0,
		0, 0, scale.z, 0,
		0, 0, 0, 1
	};
	return result;
}


//回転行列
Matrix4x4 MakeRotateXMatrix(float theta)noexcept{
	Matrix4x4 result = {
		1, 0, 0, 0,
		0, std::cos(theta), std::sin(theta), 0,
		0, -std::sin(theta), std::cos(theta), 0,
		0, 0, 0, 1
	};

	return result;
}


Matrix4x4 MakeRotateYMatrix(float theta)noexcept{
	Matrix4x4 result = {
		std::cos(theta), 0, -std::sin(theta), 0,
		0, 1, 0, 0,
		std::sin(theta), 0, std::cos(theta), 0,
		0, 0, 0, 1
	};
	return result;
}

Matrix4x4 MakeRotateZMatrix(float theta)noexcept {
	Matrix4x4 result = {
		std::cos(theta),std::sin(theta),0,0,
		-std::sin(theta),std::cos(theta),0,0,
		0,0,1,0,
		0,0,0,1
	};
	return result;
}

Matrix4x4 MakeAffineMatrix(const Vector3& scale, const Vector3& rotate, const Vector3& translate)noexcept{
	Matrix4x4 affineMatrix;
	Matrix4x4 translateMatrix = Cx::Math::MakeTranslateMatrix(translate);
	Matrix4x4 scaleMatrix = Cx::Math::MakeScaleMatrix(scale);

	Matrix4x4 rotateXMatrix = Cx::Math::MakeRotateXMatrix(rotate.x);
	Matrix4x4 rotateYMatrix = Cx::Math::MakeRotateYMatrix(rotate.y);
	Matrix4x4 rotateZMatrix = Cx::Math::MakeRotateZMatrix(rotate.z);
	Matrix4x4 rotateMatrix = Matrix4x4::Multiply(Matrix4x4::Multiply(rotateXMatrix,rotateYMatrix),rotateZMatrix);

	affineMatrix = Matrix4x4::Multiply(Matrix4x4::Multiply(scaleMatrix,rotateMatrix),translateMatrix);

	return affineMatrix;
}


Matrix4x4 MakeAffineMatrix(const Vector3& scale, const Quaternion& rotate, const Vector3& translate)noexcept{
	// 各種変換行列を生成
	const Matrix4x4 scaleMatrix = Cx::Math::MakeScaleMatrix(scale);
	const Matrix4x4 rotationMatrix = Quaternion::ToMatrix(rotate);
	const Matrix4x4 translationMatrix = Cx::Math::MakeTranslateMatrix(translate);

	// スケーリング → 回転 → 平行移動 の順で合成
	Matrix4x4 affineMatrix = Matrix4x4::Multiply(
												 Matrix4x4::Multiply(scaleMatrix,rotationMatrix),
												 translationMatrix
												);

	return affineMatrix;
}

Matrix4x4 MakeOrthographicMatrix(float l, float t, float r, float b, float nearClip, float farClip)noexcept{
	Matrix4x4 result;
	result = {
		2 / (r - l), 0, 0, 0,
		0, 2 / (t - b), 0, 0,
		0, 0, 1 / (farClip - nearClip), 0,
		(l + r) / (l - r), (t + b) / (b - t), nearClip / (nearClip - farClip), 1
	};
	return result;
}


Vector3 TransformNormal(const Vector3& v, const Matrix4x4& m)noexcept{
	Vector3 result{
		v.x * m.m[0][0] + v.y * m.m[1][0] + v.z * m.m[2][0],
		v.x * m.m[0][1] + v.y * m.m[1][1] + v.z * m.m[2][1],
		v.x * m.m[0][2] + v.y * m.m[1][2] + v.z * m.m[2][2]
	};
	return result;
}

float Lerp(float v1, float v2, float t)noexcept{ return v1 + (v2 - v1) * t; }

float LerpShortAngle(float a, float b, float t) noexcept{
	constexpr float PI = 3.14159265358979323846f;
	constexpr float TAU = 2.0f * PI;
	float d = std::fmod(b - a,TAU);
	if (d > PI) d -= TAU;
	else if (d < -PI) d += TAU;
	return a + d * t;
}

Vector2 WorldToScreen(const Vector3& worldPos) {
	const Matrix4x4& viewProj = CameraManager::GetMain3d()->GetViewProjectionMatrix();

	// ワールド→クリップ空間
	Vector4 clipPos = Vector4::Transform(Vector4(worldPos, 1.0f),viewProj);

	if (fabs(clipPos.w) < 1e-5f) {
		return Vector2(0.0f, 0.0f); // 無効値
	}

	// NDC座標へ
	Vector3 ndcPos = {
		clipPos.x / clipPos.w,
		clipPos.y / clipPos.w,
		clipPos.z / clipPos.w
	};

	// NDC → スクリーン座標
	float screenWidth = kGameWidth;
	float screenHeight = kGameHeight;

	float screenX = (ndcPos.x * 0.5f + 0.5f) * screenWidth;
	float screenY = (1.0f - (ndcPos.y * 0.5f + 0.5f)) * screenHeight;

	return Vector2(screenX, screenY);
}

Vector3 ScreenToWorld(const Vector2& screenPos, float depthZ) {
	float screenWidth = kGameWidth;
	float screenHeight = kGameHeight;

	// スクリーン座標 → NDC座標に変換
	float ndcX = (screenPos.x / screenWidth) * 2.0f - 1.0f;
	float ndcY = 1.0f - (screenPos.y / screenHeight) * 2.0f;

	// NDC → ワールド座標（逆 ViewProjection 行列を使う）
	Vector4 ndcPos(ndcX, ndcY, depthZ, 1.0f);

	Matrix4x4 invViewProj = Matrix4x4::Inverse(CameraManager::GetMain3d()->GetViewProjectionMatrix());
	Vector4 worldH = Vector4::Transform(ndcPos,invViewProj );

	if (fabs(worldH.w) > 1e-5f) {
		worldH.x /= worldH.w;
		worldH.y /= worldH.w;
		worldH.z /= worldH.w;
	}

	return Vector3(worldH.x, worldH.y, worldH.z);
}
}