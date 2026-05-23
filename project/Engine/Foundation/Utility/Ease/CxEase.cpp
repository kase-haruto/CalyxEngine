#include "CxEase.h"
#include "Ease.h"
#include <algorithm>
#include <externals/imgui/imgui.h>

namespace CalyxEngine {

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
			return CalyxEngine::Linear(t);
		case EaseType::EaseInQuad:
			return CalyxEngine::EaseInQuad(t);
		case EaseType::EaseOutQuad:
			return CalyxEngine::EaseOutQuad(t);
		case EaseType::EaseInOutQuad:
			return CalyxEngine::EaseInOutQuad(t);
		case EaseType::EaseInCubic:
			return CalyxEngine::EaseInCubic(t);
		case EaseType::EaseOutCubic:
			return CalyxEngine::EaseOutCubic(t);
		case EaseType::EaseInOutCubic:
			return CalyxEngine::EaseInOutCubic(t);
		case EaseType::EaseInSine:
			return CalyxEngine::EaseInSine(t);
		case EaseType::EaseOutSine:
			return CalyxEngine::EaseOutSine(t);
		case EaseType::EaseInOutSine:
			return CalyxEngine::EaseInOutSine(t);
		case EaseType::EaseInExpo:
			return CalyxEngine::EaseInExpo(t);
		case EaseType::EaseOutExpo:
			return CalyxEngine::EaseOutExpo(t);
		case EaseType::EaseInOutExpo:
			return CalyxEngine::EaseInOutExpo(t);
		case EaseType::EaseInBack:
			return CalyxEngine::EaseInBack(t);
		case EaseType::EaseOutBack:
			return CalyxEngine::EaseOutBack(t);
		case EaseType::EaseInOutBack:
			return CalyxEngine::EaseInOutBack(t);
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

		CalyxEngine::EaseType tmp = static_cast<CalyxEngine::EaseType>(easeInt);

		ImGui::PushID(label);
		if (CalyxEngine::SelectEase(tmp)) {
			easeInt = static_cast<int32_t>(tmp);
			changed = true;
		}
		ImGui::PopID();

		return changed;
	}

} // namespace CalyxEngine