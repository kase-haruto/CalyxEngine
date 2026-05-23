#include "IEngineUI.h"

namespace CalyxEngine {
	IEngineUI::IEngineUI(const std::string& name)
		: panelName_(name) {}

	IEngineUI::IEngineUI() : panelName_("Unnamed") {}

}
