#include "FxGuiHelpers.h"

#include <Engine/Application/Effects/Particle/Parm/FxParm.h>

#include <externals/imgui/imgui.h>
#include <externals/imgui/imgui_internal.h>

namespace CalyxEffect {

	namespace FxGui {

		//---------------------- GridScope 実装 ----------------------
		GridScope::GridScope(const char* label, bool defaultOpen) {
			ImGuiTreeNodeFlags f = ImGuiTreeNodeFlags_SpanAvailWidth | (defaultOpen ? ImGuiTreeNodeFlags_DefaultOpen : 0);

			// セクションの開閉
			open = ImGui::CollapsingHeader(label, f);
			if(!open) {
				began_table = false;
				return;
			}

			// 以降はテーブル生成に成功したときだけ Push を保持
			ImGui::PushID(label);
			ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(6, 6));

			began_table = ImGui::BeginTable("grid", 2,
											ImGuiTableFlags_SizingStretchSame | ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersInnerV);
			if(began_table) {
				ImGui::TableSetupColumn("Prop", ImGuiTableColumnFlags_WidthFixed, 140.0f);
				ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch);
			} else {
				// BeginTable 失敗時は即リセットして open=false 扱いに
				ImGui::PopStyleVar();
				ImGui::PopID();
				open = false;
			}
		}

		GridScope::~GridScope() {
			// BeginTable 成功時のみ EndTable/Pop
			if(began_table) {
				ImGui::EndTable();
				ImGui::PopStyleVar();
				ImGui::PopID();
			}
		}

		//---------------------- RowLabel （安全版） ----------------------
		void RowLabel(const char* name) {
			if(ImGui::GetCurrentTable() != nullptr) {
				ImGui::TableNextRow();
				ImGui::TableSetColumnIndex(0);
				ImGui::AlignTextToFramePadding();
				ImGui::TextUnformatted(name);
				ImGui::TableSetColumnIndex(1);
			} else {
				// テーブル外でも落ちないフォールバック
				ImGui::AlignTextToFramePadding();
				ImGui::TextUnformatted(name);
				ImGui::SameLine();
			}
		}

		//---------------------- DrawModeCombo ----------------------
		bool DrawModeCombo(const char* id, FxValueMode& m) {
			int			cur		= (m == FxValueMode::Constant) ? 0
															   : (m == FxValueMode::Random ? 1 : 2);
			const char* items[] = {"Constant", "Random(Box)", "RandomSphere"};
			if(ImGui::Combo(id, &cur, items, IM_ARRAYSIZE(items))) {
				m = (cur == 0) ? FxValueMode::Constant
							   : (cur == 1 ? FxValueMode::Random : FxValueMode::RandomSphere);
				return true;
			}
			return false;
		}

	} // namespace FxGui

} // namespace CalyxEffect