#pragma once
/* ========================================================================
/* include space
/* ===================================================================== */
#include <Engine/Application/UI/EngineUI/IOnViewportTool.h>

/*-----------------------------------------------------------------------------------------
 * DebugTextOverlay
 * - デバッグテキストオーバーレイクラス
 * - エンジン画面右下にデバッグトーストを表示するためのクラス
 *---------------------------------------------------------------------------------------*/
namespace CalyxEngine {
	/**
	 * @brief DebugTextOverlayの機能を提供するクラスです。
	 */
	class DebugTextOverlay final
		: public BaseOnViewportTool {
	public:
		//===================================================================*/
		//					methods
		//===================================================================*/
		DebugTextOverlay();
		~DebugTextOverlay() override;

		void RenderOverlay(const ImVec2& basePos) override;
		static void RenderGlobalPopups();
		static void RenderFatalAssertWindow();

	private:
		//===================================================================*/
		//					fields
		//===================================================================*/
	};
} // namespace CalyxEngine
