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
	// Popupは秒単位の寿命と上昇量を保持し、描画側が現在ageから位置・透明度を計算する。
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
	// 最初のFatal Assertを保持し、連鎖Assertで原因情報が上書きされるのを防ぐ。
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
	// 要求は一度だけ消費し、Debugger Break後に同じFrameで再発火しないようResetする。
	instance.breakRequested_ = false;
	if(requested) {
		instance.hasFatalAssert_ = false;
	}
	return requested;
}

void DebugTextManager::UpdatePopupTexts(float dt) {
	auto& popups = GetInstance().popupTexts_;
	// 各Popupの経過秒を進めてから、寿命へ到達した要素をまとめて除去する。
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
	// 常設MessageだけをFrame末尾に消去し、寿命管理されるPopupは保持する。
	GetInstance().messages_.clear();
}

DebugTextManager& DebugTextManager::GetInstance() {
	static DebugTextManager instance;
	return instance;
}

} // namespace CalyxEngine
