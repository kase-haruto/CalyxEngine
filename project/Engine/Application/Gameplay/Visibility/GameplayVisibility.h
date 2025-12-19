#pragma once
#include <Engine/Graphics/Camera/3d/Camera3d.h>
#include <Engine/Foundation/Math/Vector3.h>
#include <Engine/Foundation/Math/Vector4.h>
#include <Engine/Foundation/Math/Matrix4x4.h>
#include <Engine/Objects/3D/Geometory/AABB.h>

namespace GameplayVisibility {

// AABB の8頂点
	inline void GetCorners(const AABB& b, CxMath::Vector3 out[8]) {
		const CxMath::Vector3& mn = b.min_;
		const CxMath::Vector3& mx = b.max_;
		out[0] = { mn.x, mn.y, mn.z };
		out[1] = { mx.x, mn.y, mn.z };
		out[2] = { mn.x, mx.y, mn.z };
		out[3] = { mx.x, mx.y, mn.z };
		out[4] = { mn.x, mn.y, mx.z };
		out[5] = { mx.x, mn.y, mx.z };
		out[6] = { mn.x, mx.y, mx.z };
		out[7] = { mx.x, mx.y, mx.z };
	}

	inline CxMath::Matrix4x4 GetViewProj(const Camera3d* cam) {
		return cam->GetViewProjectionMatrix();
	}

	// 画面内（NDC）チェック。padNdc=0.05f なら左右上下 5% 内側のみを「画面内」とする
	inline bool IsAabbOnScreenNdc(const Camera3d* cam, const AABB& worldAabb, float padNdc = 0.05f) {
		if (!cam) return true;
		const CxMath::Matrix4x4 vp = GetViewProj(cam);

		CxMath::Vector3 corners[8]; GetCorners(worldAabb, corners);

		const float nx = 1.0f - padNdc; // [-nx, +nx] に入ればOK
		const float ny = 1.0f - padNdc;

		for (int i = 0; i < 8; ++i) {
			const CxMath::Vector3& p = corners[i];
			const CxMath::Vector4 cp = vp * CxMath::Vector4(p.x, p.y, p.z, 1.0f);
			if (cp.w <= 1e-6f) continue; // 背面など

			const float invw = 1.0f / cp.w;
			const float x = cp.x * invw;
			const float y = cp.y * invw;
			const float z = cp.z * invw; // DirectX: [0,1]

			if (x >= -nx && x <= nx &&
				y >= -ny && y <= ny &&
				z >= 0.0f && z <= 1.0f) {
				return true; // ← 重複していた return を整理
			}
		}
		return false;
	}

	// 射程チェック（距離の二乗で比較）
	inline bool InEngageRangeSq(const CxMath::Vector3& from, const CxMath::Vector3& to, float maxDist) {
		const CxMath::Vector3 d = to - from;
		const float d2 = d.x * d.x + d.y * d.y + d.z * d.z;
		return d2 <= (maxDist * maxDist);
	}

} // namespace GameplayVisibility