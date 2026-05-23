#include "ShadowBounds.h"

#include "Engine/Foundation/Utility/Func/CxUtils.h"

namespace CalyxEngine {

	void ShadowBounds::UpdateFromCamera(const Camera3d& camera,float shadowFar,float expandMargin) {
		// -----------------------------
		// カメラのワールド位置
		// -----------------------------
		const CalyxEngine::Vector3 camPos = camera.GetWorldTransform().GetWorldPosition();

		// -----------------------------
		// カメラの Forward
		// -----------------------------
		CalyxEngine::Vector3 camForward = camera.GetForward();

		// -----------------------------
		// Shadow AABB の中心
		// （カメラ前方に half shadowFar）
		// -----------------------------
		CalyxEngine::Vector3 center =
			camPos + camForward * (shadowFar * 0.5f);

		// -----------------------------
		// Shadow AABB の半サイズ
		// -----------------------------
		CalyxEngine::Vector3 halfSize(
			shadowFar * 0.5f,
			shadowFar*2, // 高さは少し余裕
			shadowFar * 0.5f
			);

		// expandMargin を加算
		halfSize += CalyxEngine::Vector3(expandMargin);

		// -----------------------------
		// AABB 設定
		// -----------------------------
		bounds_.min_ = center - halfSize;
		bounds_.max_ = center + halfSize;
	}
}