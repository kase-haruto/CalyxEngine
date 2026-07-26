#pragma once
#include "../EngineUI/IEngineUI.h"

namespace CalyxEngine {

	/**
	 * @brief LivePPPanelの機能を提供するクラスです。
	 */
	class LivePPPanel : public IEngineUI {
	public:
		LivePPPanel();
		~LivePPPanel() override = default;

		void Render() override;
	};

} // namespace CalyxEngine
