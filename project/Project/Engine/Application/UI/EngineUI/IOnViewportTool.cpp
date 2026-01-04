#include "IOnViewportTool.h"

ImVec2 BaseOnViewportTool::CalcScreenPosition(const ImVec2& viewportPos,
											  const ImVec2& viewportSize) const{
	ImVec2 anchor;

	switch (align_){
		case OverlayAlign::TopLeft:
			anchor = ImVec2(0.0f, 0.0f);
			break;
		case OverlayAlign::TopRight:
			anchor = ImVec2(viewportSize.x, 0.0f);
			break;
		case OverlayAlign::BottomLeft:
			anchor = ImVec2(0.0f, viewportSize.y);
			break;
		case OverlayAlign::BottomRight:
			anchor = ImVec2(viewportSize.x, viewportSize.y);
			break;
		case OverlayAlign::CenterTop:
			anchor = ImVec2(viewportSize.x * 0.5f, 0.0f);
			break;
		default:
			anchor = ImVec2(0.0f, 0.0f); // fallback safety
			break;
	}

	// 最終的なスクリーン座標 = Viewport 左上 + Anchor 相対位置 + ツール独自のオフセット
	return ImVec2(viewportPos.x + anchor.x + overlayOffset_.x,
				  viewportPos.y + anchor.y + overlayOffset_.y);
}