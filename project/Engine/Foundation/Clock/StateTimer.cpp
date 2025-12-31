#include "StateTimer.h"

// engine
#include <Engine/Foundation/Utility/Converter/EnumConverter.h>
#include <Engine/Foundation/Utility/Ease/CxEase.h>

// c++
#include "imgui/imgui.h"
#include <algorithm>

namespace CalyxUtil {

	//============================================================================
	// constructor
	//============================================================================
	StateTimer::StateTimer(float target, CalyxEase::EaseType easing)
		: target_(std::max(0.0001f, target)),
		  easingType_(easing) {
		Reset();
	}

	//============================================================================
	// update
	//============================================================================
	void StateTimer::Update(float deltaTime, const std::optional<float>& newTarget) {

		// target が外部から更新された場合
		if(newTarget.has_value()) {
			SetTarget(*newTarget);
		}

		// すでに到達していれば更新しない
		if(IsReached()) {
			return;
		}

		// 時間を進める
		current_ += deltaTime;

		// 正規化T更新
		UpdateT();

		// イージング更新
		UpdateEasing();
	}

	//============================================================================
	// control
	//============================================================================
	void StateTimer::Reset() {

		current_ = 0.0f;
		t_		 = 0.0f;
		easedT_	 = 0.0f;
	}

	void StateTimer::SetTarget(float target) {

		target_ = std::max(0.0001f, target);
		UpdateT();
		UpdateEasing();
	}

	void StateTimer::SetEasing(CalyxEase::EaseType type) {

		easingType_ = type;
		UpdateEasing();
	}

	//============================================================================
	// query
	//============================================================================
	bool StateTimer::IsReached() const {

		return current_ >= target_;
	}

	bool StateTimer::IsRunning() const {

		return current_ < target_;
	}

	//============================================================================
	// internal
	//============================================================================
	void StateTimer::UpdateT() {

		t_ = std::clamp(current_ / target_, 0.0f, 1.0f);
	}

	void StateTimer::UpdateEasing() {
		easedT_ = CalyxEase::ApplyEase(easingType_, t_);
	}

	//============================================================================
	// editor
	//============================================================================
	void StateTimer::ImGui(const std::string& label, bool separate) {

		if(separate) {
			ImGui::SeparatorText(label.c_str());
		}

		ImGui::DragFloat("Current", &current_, 0.01f, 0.0f, target_);
		ImGui::DragFloat("Target", &target_, 0.01f, 0.0001f, 100.0f);

		ImGui::Text("T      : %.3f", t_);
		ImGui::Text("EasedT : %.3f", easedT_);

		// イージングタイプ選択
		const char* easeItems[static_cast<int>(CalyxEase::EaseType::Count)];
		// enum の個数分
		for(int i = 0; i < static_cast<int>(CalyxEase::EaseType::Count); ++i) {
			easeItems[i] = EnumConverter<CalyxEase::EaseType>::ToString(static_cast<CalyxEase::EaseType>(i)).data();
		}
		int easeIdx = static_cast<int>(easingType_);
		if(ImGui::Combo("Easing Type", &easeIdx, easeItems, static_cast<int>(CalyxEase::EaseType::Count))) {
			easingType_ = static_cast<CalyxEase::EaseType>(easeIdx);
			UpdateEasing();
		}
	}

} // namespace CalyxUtil
