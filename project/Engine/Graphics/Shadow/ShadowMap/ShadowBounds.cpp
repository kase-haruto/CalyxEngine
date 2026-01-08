#include "ShadowBounds.h"

#include "Engine/Foundation/Utility/Func/CxUtils.h"

namespace CalyxGraphics {

	void ShadowBounds::UpdateFromCamera(const Camera3d& camera,float shadowFar,float expandMargin) {
		// -----------------------------
		// カメラのワールド位置
		// -----------------------------
		const CalyxMath::Vector3 camPos = camera.GetWorldTransform().GetWorldPosition();

		// -----------------------------
		// カメラの Forward
		// -----------------------------
		CalyxMath::Vector3 camForward = camera.GetForward();

		// -----------------------------
		// Shadow AABB の中心
		// （カメラ前方に half shadowFar）
		// -----------------------------
		CalyxMath::Vector3 center =
			camPos + camForward * (shadowFar * 0.5f);

		// -----------------------------
		// Shadow AABB の半サイズ
		// -----------------------------
		CalyxMath::Vector3 halfSize(
			shadowFar * 0.5f,
			shadowFar*2, // 高さは少し余裕
			shadowFar * 0.5f
			);

		// expandMargin を加算
		halfSize += CalyxMath::Vector3(expandMargin);

		// -----------------------------
		// AABB 設定
		// -----------------------------
		bounds_.min_ = center - halfSize;
		bounds_.max_ = center + halfSize;
	}
}