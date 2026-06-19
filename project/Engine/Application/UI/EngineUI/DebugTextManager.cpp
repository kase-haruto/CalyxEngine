#include "DebugTextManager.h"

#include <algorithm>

namespace CalyxEngine {

void DebugTextManager::AddMessage(const std::string& title, const std::string& body, const ImVec4& color) {
	GetInstance().messages_.push_back({title, body, color});
}

void DebugTextManager::AddPopupText(const CalyxEngine::Vector2& position,
									const ImVec4&			   color,
									const std::string&		   text,
									float					   lifetime,
									float					   rise) {
	if(text.empty()) return;
	GetInstance().popupTexts_.push_back({position, color, text, lifetime, 0.0f, rise});
}

const std::vector<DebugTextManager::Message>& DebugTextManager::GetMessages() {
	return GetInstance().messages_;
}

const std::vector<DebugTextManager::PopupText>& DebugTextManager::GetPopupTexts() {
	return GetInstance().popupTexts_;
}

void DebugTextManager::SetFatalAssert(const FatalAssert& fatal) {
	auto& instance = GetInstance();
	if(instance.hasFatalAssert_) return;
	instance.fatalAssert_ = fatal;
	instance.hasFatalAssert_ = true;
	instance.breakRequested_ = false;
}

bool DebugTextManager::HasFatalAssert() {
	return GetInstance().hasFatalAssert_;
}

const DebugTextManager::FatalAssert& DebugTextManager::GetFatalAssert() {
	return GetInstance().fatalAssert_;
}

void DebugTextManager::RequestBreak() {
	GetInstance().breakRequested_ = true;
}

bool DebugTextManager::ConsumeBreakRequest() {
	auto&	   instance  = GetInstance();
	const bool requested = instance.breakRequested_;
	instance.breakRequested_ = false;
	if(requested) {
		instance.hasFatalAssert_ = false;
	}
	return requested;
}

void DebugTextManager::UpdatePopupTexts(float dt) {
	auto& popups = GetInstance().popupTexts_;
	for(auto& popup : popups) {
		popup.age += dt;
	}

	popups.erase(
		std::remove_if(
			popups.begin(),
			popups.end(),
			[](const PopupText& popup) {
				return popup.age >= popup.lifetime;
			}),
		popups.end());
}

void DebugTextManager::Clear() {
	GetInstance().messages_.clear();
}

DebugTextManager& DebugTextManager::GetInstance() {
	static DebugTextManager instance;
	return instance;
}

} // namespace CalyxEngine
