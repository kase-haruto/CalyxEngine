#pragma once
#include "../EngineUI/IEngineUI.h"

namespace CalyxEngine {

	class LivePPPanel : public IEngineUI {
	public:
		LivePPPanel();
		~LivePPPanel() override = default;

		void Render() override;
	};

} // namespace CalyxEngine
