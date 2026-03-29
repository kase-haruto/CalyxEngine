#pragma once
#include "../EngineUI/IEngineUI.h"

namespace CalyxEditor {

	class LivePPPanel : public IEngineUI {
	public:
		LivePPPanel();
		~LivePPPanel() override = default;

		void Render() override;
	};

} // namespace CalyxEditor
