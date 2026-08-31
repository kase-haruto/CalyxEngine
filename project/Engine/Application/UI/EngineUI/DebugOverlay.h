#pragma once
#include <Engine/Application/UI/EngineUI/IOnViewportTool.h>

namespace CalyxEngine {

	/*-----------------------------------------------------------------------------------------
	 * DebugOverlay
	 * - デバッグ用オーバーレイ表示
	 * - DebugTextManager に登録されたメッセージをビューポート上に描画する
	 *---------------------------------------------------------------------------------------*/
	/**
	 * @brief DebugOverlayの機能を提供するクラスです。
	 */
	class DebugOverlay : public BaseOnViewportTool {
	public:
		DebugOverlay();
		~DebugOverlay() override = default;

		void RenderOverlay(const ImVec2& basePos) override;
		void RenderToolbar() override {};
	};

} // namespace CalyxEngine
