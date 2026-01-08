#include "Frustum.h"

#include <Engine/Renderer/Primitive/PrimitiveDrawer.h>

void Frustum::ExtractFromMatrix(const CalyxMath::Matrix4x4& viewProj){
	viewProjection_ = viewProj;
	const auto& m = viewProj.m;

	planes_[0] = NormalizePlane({m[0][3] + m[0][0], m[1][3] + m[1][0], m[2][3] + m[2][0], m[3][3] + m[3][0]}); // Left
	planes_[1] = NormalizePlane({m[0][3] - m[0][0], m[1][3] - m[1][0], m[2][3] - m[2][0], m[3][3] - m[3][0]}); // Right
	planes_[2] = NormalizePlane({m[0][3] + m[0][1], m[1][3] + m[1][1], m[2][3] + m[2][1], m[3][3] + m[3][1]}); // Bottom
	planes_[3] = NormalizePlane({m[0][3] - m[0][1], m[1][3] - m[1][1], m[2][3] - m[2][1], m[3][3] - m[3][1]}); // Top
	planes_[4] = NormalizePlane({m[0][3] + m[0][2], m[1][3] + m[1][2], m[2][3] + m[2][2], m[3][3] + m[3][2]}); // Near
	planes_[5] = NormalizePlane({m[0][3] - m[0][2], m[1][3] - m[1][2], m[2][3] - m[2][2], m[3][3] - m[3][2]}); // Far
}

bool Frustum::IsAABBInside(const CalyxMath::Vector3& min, const CalyxMath::Vector3& max) const{
	for (const auto& plane : planes_){
		CalyxMath::Vector3 positive = {
			(plane.normal.x >= 0) ? max.x : min.x,
			(plane.normal.y >= 0) ? max.y : min.y,
			(plane.normal.z >= 0) ? max.z : min.z
		};

		if (plane.GetSignedDistanceToPoint(positive) < 0.0f){
			return false;
		}
	}
	return true;
}

void Frustum::Draw(const CalyxMath::Vector4& color, float farPlaneRatio) const {
	CalyxMath::Vector3 corners[8];
	CalculateCorners(corners);

	// ----------  ここで遠平面を手前に寄せる ----------
	if (farPlaneRatio < 1.f) {
		for (int i = 0; i < 4; ++i) {
			// near[i] から far[i] 方向へのベクトル
			CalyxMath::Vector3 v = corners[i + 4] - corners[i];
			corners[i + 4] = corners[i] + v * farPlaneRatio;   // 圧縮
		}
	}

	// ---------- 描画 ----------
	auto draw = PrimitiveDrawer::GetInstance();
	// near
	draw->DrawLine3d(corners[0], corners[1], color);
	draw->DrawLine3d(corners[1], corners[2], color);
	draw->DrawLine3d(corners[2], corners[3], color);
	draw->DrawLine3d(corners[3], corners[0], color);
	// far
	draw->DrawLine3d(corners[4], corners[5], color);
	draw->DrawLine3d(corners[5], corners[6], color);
	draw->DrawLine3d(corners[6], corners[7], color);
	draw->DrawLine3d(corners[7], corners[4], color);
	// sides
	draw->DrawLine3d(corners[0], corners[4], color);
	draw->DrawLine3d(corners[1], corners[5], color);
	draw->DrawLine3d(corners[2], corners[6], color);
	draw->DrawLine3d(corners[3], corners[7], color);
}

void Frustum::CalculateCorners(CalyxMath::Vector3 outCorners[8]) const{
	CalyxMath::Matrix4x4 inv = CalyxMath::Matrix4x4::Inverse(viewProjection_);
	CalyxMath::Vector3 ndc[8] = {
		{-1, -1, 0}, {1, -1, 0}, {1, 1, 0}, {-1, 1, 0},
		{-1, -1, 1}, {1, -1, 1}, {1, 1, 1}, {-1, 1, 1}
	};

	for (int i = 0; i < 8; ++i){
		CalyxMath::Vector4 clip(ndc[i].x, ndc[i].y, ndc[i].z, 1.0f);
		CalyxMath::Vector4 world = CalyxMath::Vector4::Transform(clip, inv);
		outCorners[i] = CalyxMath::Vector3(world.x / world.w, world.y / world.w, world.z / world.w);
	}
}

void Frustum::CalculateCorners(CalyxMath::Vector3 outCorners[8], float farPlaneRatio) const {
	CalculateCorners(outCorners); // 既存の8点（near/farそのまま）

	if (farPlaneRatio < 1.0f) {
		for (int i = 0; i < 4; ++i) {
			CalyxMath::Vector3 v = outCorners[i + 4] - outCorners[i];
			outCorners[i + 4] = outCorners[i] + v * farPlaneRatio;
		}
	}
}

FrustumPlane Frustum::NormalizePlane(const CalyxMath::Vector4& p){
	CalyxMath::Vector3 n = {p.x, p.y, p.z};
	float len = n.Length();
	return {n / len, p.w / len};
}