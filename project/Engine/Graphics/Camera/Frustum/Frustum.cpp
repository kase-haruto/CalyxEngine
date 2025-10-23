#include "Frustum.h"

#include <Engine/Renderer/Primitive/PrimitiveDrawer.h>

/////////////////////////////////////////////////////////////////////////////////////////
//		行列から視錐台（Frustum）を抽出
/////////////////////////////////////////////////////////////////////////////////////////
void Frustum::ExtractFromMatrix(const Matrix4x4& viewProj) {
	// ViewProjection行列を保存
	viewProjection_ = viewProj;
	const auto& m	= viewProj.m;

	// それぞれの結果は平面方程式 ax + by + cz + d = 0 の係数 (a,b,c,d)
	planes_[0] = NormalizePlane({m[0][3] + m[0][0], m[1][3] + m[1][0], m[2][3] + m[2][0], m[3][3] + m[3][0]}); // Left
	planes_[1] = NormalizePlane({m[0][3] - m[0][0], m[1][3] - m[1][0], m[2][3] - m[2][0], m[3][3] - m[3][0]}); // Right
	planes_[2] = NormalizePlane({m[0][3] + m[0][1], m[1][3] + m[1][1], m[2][3] + m[2][1], m[3][3] + m[3][1]}); // Bottom
	planes_[3] = NormalizePlane({m[0][3] - m[0][1], m[1][3] - m[1][1], m[2][3] - m[2][1], m[3][3] - m[3][1]}); // Top
	planes_[4] = NormalizePlane({m[0][3] + m[0][2], m[1][3] + m[1][2], m[2][3] + m[2][2], m[3][3] + m[3][2]}); // Near
	planes_[5] = NormalizePlane({m[0][3] - m[0][2], m[1][3] - m[1][2], m[2][3] - m[2][2], m[3][3] - m[3][2]}); // Far
}

/////////////////////////////////////////////////////////////////////////////////////////
//		AABB（軸平行境界ボックス）が視錐台内にあるかを判定
/////////////////////////////////////////////////////////////////////////////////////////
bool Frustum::IsAABBInside(const Vector3& min, const Vector3& max) const {
	for(const auto& plane : planes_) {
		// 各平面に対して、AABBの「最も平面の外側にある可能性がある点（positive vertex）」を取得
		Vector3 positive = {
			(plane.normal.x >= 0) ? max.x : min.x,
			(plane.normal.y >= 0) ? max.y : min.y,
			(plane.normal.z >= 0) ? max.z : min.z};

		// 外側点が平面の外（負の側）にある場合 → AABBは視錐台外
		if(plane.GetSignedDistanceToPoint(positive) < 0.0f) {
			return false;
		}
	}
	return true; // すべての平面で内側 → 完全に視錐台内
}

/////////////////////////////////////////////////////////////////////////////////////////
//		視錐台を線で描画（デバッグ用）
//		farPlaneRatio < 1 で遠平面を圧縮して見やすくする
/////////////////////////////////////////////////////////////////////////////////////////
void Frustum::Draw(const Vector4& color, float farPlaneRatio) const {
	Vector3 corners[8];
	CalculateCorners(corners); // 視錐台8頂点を算出

	// ---------- 遠平面を手前に寄せる ----------
	if(farPlaneRatio < 1.f) {
		for(int i = 0; i < 4; ++i) {
			// near[i] から far[i] への方向ベクトル
			Vector3 v = corners[i + 4] - corners[i];
			// 指定比率まで手前に寄せる
			corners[i + 4] = corners[i] + v * farPlaneRatio;
		}
	}

	// ---------- 視錐台の線分描画 ----------
	auto draw = PrimitiveDrawer::GetInstance();

	// near面
	draw->DrawLine3d(corners[0], corners[1], color);
	draw->DrawLine3d(corners[1], corners[2], color);
	draw->DrawLine3d(corners[2], corners[3], color);
	draw->DrawLine3d(corners[3], corners[0], color);

	// far面
	draw->DrawLine3d(corners[4], corners[5], color);
	draw->DrawLine3d(corners[5], corners[6], color);
	draw->DrawLine3d(corners[6], corners[7], color);
	draw->DrawLine3d(corners[7], corners[4], color);

	// 側面
	draw->DrawLine3d(corners[0], corners[4], color);
	draw->DrawLine3d(corners[1], corners[5], color);
	draw->DrawLine3d(corners[2], corners[6], color);
	draw->DrawLine3d(corners[3], corners[7], color);
}

/////////////////////////////////////////////////////////////////////////////////////////
//		視錐台の8頂点を算出（NDC → World変換）
/////////////////////////////////////////////////////////////////////////////////////////
void Frustum::CalculateCorners(Vector3 outCorners[8]) const {
	// ViewProjection行列の逆行列を取得
	Matrix4x4 inv = Matrix4x4::Inverse(viewProjection_);

	// NDC空間上の立方体8点（z=0がnear, z=1がfar）
	Vector3 ndc[8] = {
		{-1, -1, 0}, {1, -1, 0}, {1, 1, 0}, {-1, 1, 0}, // near
		{-1, -1, 1},
		{1, -1, 1},
		{1, 1, 1},
		{-1, 1, 1} // far
	};

	// 各点をクリップ座標 → ワールド座標へ変換
	for(int i = 0; i < 8; ++i) {
		Vector4 clip(ndc[i].x, ndc[i].y, ndc[i].z, 1.0f);
		Vector4 world = Vector4::Transform(clip, inv);
		outCorners[i] = Vector3(world.x / world.w, world.y / world.w, world.z / world.w); // 透視除算
	}
}

/////////////////////////////////////////////////////////////////////////////////////////
//		平面の正規化（法線ベクトルを単位化）
/////////////////////////////////////////////////////////////////////////////////////////
FrustumPlane Frustum::NormalizePlane(const Vector4& p) {
	Vector3 n	= {p.x, p.y, p.z};
	float	len = n.Length();
	// 平面方程式 ax+by+cz+d=0 の a,b,c,d を正規化
	return {n / len, p.w / len};
}
