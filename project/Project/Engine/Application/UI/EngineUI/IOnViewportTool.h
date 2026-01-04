#pragma once

#include <externals/imgui/imgui.h>

enum class OverlayAlign{
	TopLeft,
	TopRight,
	BottomLeft,
	BottomRight,
	CenterTop,
};

class IOnViewportTool{
public:
	virtual ~IOnViewportTool() = default;

	virtual void RenderOverlay(const ImVec2& basePos) = 0;
	virtual void RenderToolbar() = 0;

	virtual ImVec2 GetOverlayOffset() const = 0;
};

/**
 * Viewport上に描画されるツールの共通基底クラス
 */
class BaseOnViewportTool : public IOnViewportTool{
public:
	virtual ~BaseOnViewportTool() = default;

	void SetOverlayAlign(OverlayAlign align){ align_ = align; }
	OverlayAlign GetOverlayAlign() const{ return align_; }

	void SetOverlayOffset(const ImVec2& offset){ overlayOffset_ = offset; }
	ImVec2 GetOverlayOffset() const override{ return overlayOffset_; }

	/// Viewport の左上座標とサイズから、実際に描画すべきスクリーン座標を計算
	ImVec2 CalcScreenPosition(const ImVec2& viewportPos,
							  const ImVec2& viewportSize) const;

protected:
	OverlayAlign align_ = OverlayAlign::TopLeft;
	ImVec2 overlayOffset_ = ImVec2(0.0f, 0.0f);
};
