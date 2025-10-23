#pragma once
/* ========================================================================
/* include space
/* ===================================================================== */
// engine
#include <Engine/Application/UI/EngineUI/IOnViewportTool.h>

class PerformanceOverlay
	:public BaseOnViewportTool{
public:
	//===================================================================*/
	//					methods
	//===================================================================*/
	PerformanceOverlay();
	~PerformanceOverlay() = default;

	void RenderOverlay(const ImVec2& basePos)override;
	void RenderToolbar()override;
};

