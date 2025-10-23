#pragma once

// engine
#include <Engine/Foundation/Math/Vector3.h>
#include <Engine/Foundation/Math/Matrix4x4.h>
#include <Engine/Foundation/Math/Vector4.h>
// c+
#include <array>

/* ========================================================================
/*			視推台の面
/* ===================================================================== */
struct FrustumPlane{
	Vector3 normal;
	float distance;

	float GetSignedDistanceToPoint(const Vector3& point) const{
		return Vector3::Dot(normal, point) + distance;
	}
};

/* ========================================================================
/*		視推台
/* ===================================================================== */
class Frustum{
public:
	Frustum() = default;

	/// <summary>
	/// 行列から掃き出し
	/// </summary>
	/// <param name="viewProj"></param>
	void ExtractFromMatrix(const Matrix4x4& viewProj);

	/// <summary>
	/// aabbとの判定
	/// </summary>
	/// <param name="min"></param>
	/// <param name="max"></param>
	/// <returns></returns>
	bool IsAABBInside(const Vector3& min, const Vector3& max) const;

	/// <summary>
	/// 描画
	/// </summary>
	/// <param name="color"></param>
	/// <param name="farPlaneRatio"></param>
	void Draw(const Vector4& color = Vector4(1, 1, 0, 1), float farPlaneRatio = 0.005f) const;
	
	/// <summary>
	/// 8頂点をワールド座標で計算
	/// </summary>
	/// <param name="outCorners"></param>
	void CalculateCorners(Vector3 outCorners[8]) const;

private:
	/// <summary>
	/// 面の正規化
	/// </summary>
	/// <param name="p"></param>
	/// <returns></returns>
	FrustumPlane NormalizePlane(const Vector4& p);

private:
	std::array<FrustumPlane, 6> planes_;
	Matrix4x4 viewProjection_;

	
};
