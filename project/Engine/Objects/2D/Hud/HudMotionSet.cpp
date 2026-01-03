#include "HudMotionSet.h"

#include "imgui/imgui.h"

namespace Calyx2D {

	namespace {

		//==============================================================
		// Vec2 用 GUI
		//==============================================================
		bool DrawVec2Desc(const char* name,
							 std::optional<HudMotionDesc<CalyxMath::Vector2>>& opt) {
			bool changed = false;

			bool enabled = opt.has_value();
			if(ImGui::Checkbox(name, &enabled)) {
				changed = true;
				if(enabled) opt.emplace();
				else {
					opt.reset();
					return true;
				}
			}

			if(!opt.has_value()) return changed;

			auto& d = *opt;

			if(ImGui::TreeNode(name)) {

				ImGui::DragFloat2("Start", &d.start.x, 1.0f);
				if(ImGui::IsItemDeactivatedAfterEdit()) changed = true;

				ImGui::DragFloat2("End", &d.end.x, 1.0f);
				if(ImGui::IsItemDeactivatedAfterEdit()) changed = true;

				ImGui::DragFloat("Duration", &d.duration, 0.05f, 0.01f, 10.0f);
				if(ImGui::IsItemDeactivatedAfterEdit()) changed = true;

				CalyxEase::SelectEase(d.easing);

				if(ImGui::IsItemDeactivatedAfterEdit()) changed = true;

				ImGui::TreePop();
			}

			return changed;
		}

		//==============================================================
		// float 用 GUI
		//==============================================================
		bool DrawFloatDesc(const char* name,
							  std::optional<HudMotionDesc<float>>& opt) {
			bool changed = false;

			bool enabled = opt.has_value();
			if(ImGui::Checkbox(name, &enabled)) {
				changed = true;
				if(enabled) opt.emplace();
				else {
					opt.reset();
					return true;
				}
			}

			if(!opt.has_value()) return changed;

			auto& d = *opt;

			if(ImGui::TreeNode(name)) {

				ImGui::DragFloat("Start", &d.start, 0.01f);
				if(ImGui::IsItemDeactivatedAfterEdit()) changed = true;

				ImGui::DragFloat("End", &d.end, 0.01f);
				if(ImGui::IsItemDeactivatedAfterEdit()) changed = true;

				ImGui::DragFloat("Duration", &d.duration, 0.05f, 0.01f, 10.0f);
				if(ImGui::IsItemDeactivatedAfterEdit()) changed = true;

				CalyxEase::SelectEase(d.easing);
				if(ImGui::IsItemDeactivatedAfterEdit()) changed = true;

				ImGui::TreePop();
			}

			return changed;
		}

	}

	//////////////////////////////////////////////////////////////////////////
	//		モーション編集Gui
	//////////////////////////////////////////////////////////////////////////
	bool HudMotionSet::ShowGui(const char* label) {

		ImGui::SeparatorText(label);

		bool changed = false;
		changed |= DrawVec2Desc("Position", position);
		changed |= DrawVec2Desc("Scale", scale);
		changed |= DrawFloatDesc("Rotation", rotation);
		changed |= DrawFloatDesc("Alpha", alpha);

		return changed;
	}

}