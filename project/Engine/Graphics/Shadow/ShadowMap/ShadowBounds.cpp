#include "ShadowBounds.h"

#include "Engine/Foundation/Utility/Func/CxUtils.h"

namespace CalyxGraphics {

	void ShadowBounds::UpdateFromCamera(const Camera3d& camera, float shadowFar, float expandMargin) {
		CalyxMath::Vector3 corners[8];
		camera.GetShadowFrustumCorners(corners, shadowFar);

		CalyxMath::Vector3 mn = corners[0];
		CalyxMath::Vector3 mx = corners[0];
		for (int i = 1; i < 8; ++i) {
			mn = CalyxMath::Vector3::Min(mn, corners[i]);
			mx = CalyxMath::Vector3::Max(mx, corners[i]);
		}

		// 影のはみ出し防止（外から侵入してくる影/キャスタ対策）
		mn -= CalyxMath::Vector3(expandMargin);
		mx += CalyxMath::Vector3(expandMargin);

		bounds_.min_ = mn;
		bounds_.max_ = mx;
	}
}