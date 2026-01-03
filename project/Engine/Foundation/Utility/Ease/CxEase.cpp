#include "CxEase.h"
#include "Ease.h"
#include <algorithm>
#include <externals/imgui/imgui.h>

namespace CalyxEase {

	//==============================================================
	//  イージング名リスト
	//==============================================================
	const char* EaseTypeNames[static_cast<int>(EaseType::Count)] = {
		"Linear",
		"EaseInQuad", "EaseOutQuad", "EaseInOutQuad",
		"EaseInCubic", "EaseOutCubic", "EaseInOutCubic",
		"EaseInSine", "EaseOutSine", "EaseInOutSine",
		"EaseInExpo", "EaseOutExpo", "EaseInOutExpo",
		"EaseInBack", "EaseOutBack", "EaseInOutBack"};

	//==============================================================
	//  イージング補完
	//==============================================================
	float EaseLerp(float start, float end, float t, EaseType ease) {
		// 0〜1 の補間率にクランプ
		t = std::clamp(t, 0.0f, 1.0f);

		// イージングを適用（
		float e = ApplyEase(ease, t);

		return start + (end - start) * e;
	}

	//==============================================================
	//  イージング計算
	//==============================================================
	float ApplyEase(EaseType type, float t) {
		switch(type) {
		case EaseType::Linear:
			return CalyxEase::Linear(t);
		case EaseType::EaseInQuad:
			return CalyxEase::EaseInQuad(t);
		case EaseType::EaseOutQuad:
			return CalyxEase::EaseOutQuad(t);
		case EaseType::EaseInOutQuad:
			return CalyxEase::EaseInOutQuad(t);
		case EaseType::EaseInCubic:
			return CalyxEase::EaseInCubic(t);
		case EaseType::EaseOutCubic:
			return CalyxEase::EaseOutCubic(t);
		case EaseType::EaseInOutCubic:
			return CalyxEase::EaseInOutCubic(t);
		case EaseType::EaseInSine:
			return CalyxEase::EaseInSine(t);
		case EaseType::EaseOutSine:
			return CalyxEase::EaseOutSine(t);
		case EaseType::EaseInOutSine:
			return CalyxEase::EaseInOutSine(t);
		case EaseType::EaseInExpo:
			return CalyxEase::EaseInExpo(t);
		case EaseType::EaseOutExpo:
			return CalyxEase::EaseOutExpo(t);
		case EaseType::EaseInOutExpo:
			return CalyxEase::EaseInOutExpo(t);
		case EaseType::EaseInBack:
			return CalyxEase::EaseInBack(t);
		case EaseType::EaseOutBack:
			return CalyxEase::EaseOutBack(t);
		case EaseType::EaseInOutBack:
			return CalyxEase::EaseInOutBack(t);
		default:
			return t;
		}
	}

	//==============================================================
	//  ImGuiでイージングを選択
	//==============================================================
	bool SelectEase(EaseType& type) {
		bool changed = false;
		int current = static_cast<int>(type);

		if (ImGui::BeginCombo("Ease Type", EaseTypeNames[current])) {
			for (int i = 0; i < static_cast<int>(EaseType::Count); ++i) {
				bool isSelected = (current == i);
				if (ImGui::Selectable(EaseTypeNames[i], isSelected)) {
					if (current != i) {
						type = static_cast<EaseType>(i);
						changed = true;
					}
				}
				if (isSelected)
					ImGui::SetItemDefaultFocus();
			}
			ImGui::EndCombo();
		}

		return changed;
	}

	bool SelectEaseInt(const char* label, int32_t& easeInt) {
		bool changed = false;

		CalyxEase::EaseType tmp = static_cast<CalyxEase::EaseType>(easeInt);

		ImGui::PushID(label);
		if (CalyxEase::SelectEase(tmp)) {
			easeInt = static_cast<int32_t>(tmp);
			changed = true;
		}
		ImGui::PopID();

		return changed;
	}

} // namespace CalyxEase