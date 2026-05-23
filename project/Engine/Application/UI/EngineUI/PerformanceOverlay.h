#pragma once
/* ========================================================================
/* include space
/* ===================================================================== */
// engine
#include <Engine/Application/UI/EngineUI/IOnViewportTool.h>

namespace CalyxEngine {

	/*-----------------------------------------------------------------------------------------
	 * PerformanceOverlay
	 * - パフォーマンスオーバーレイクラス
	 * - FPS・描画時間などのパフォーマンス情報をビューポート上に表示
	 *---------------------------------------------------------------------------------------*/
	class PerformanceOverlay
		: public BaseOnViewportTool {
	public:
		//===================================================================*/
		//					methods
		//===================================================================*/
		PerformanceOverlay();
		~PerformanceOverlay() = default;

		void RenderOverlay(const ImVec2& basePos) override;
		void RenderToolbar() override;

	private:
		bool   showOverlay_	 = true;
		bool   isAdjustment_ = false; //< 調整中か
		ImVec4 color_		 = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
	};
} // namespace CalyxEngine