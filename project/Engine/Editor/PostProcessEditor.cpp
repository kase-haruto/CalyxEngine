#include "PostProcessEditor.h"

#include <Engine/PostProcess/Graph/PostEffectGraph.h>
#include <Engine/PostProcess/Slot/PostEffectSlot.h>
#include <Engine/PostProcess/Collection/PostProcessCollection.h>

#include <externals/imgui/imgui.h>
namespace CalyxEditor {
	PostProcessEditor::PostProcessEditor(const std::string& name)
		: BaseEditor(name) {}

	void PostProcessEditor::ShowImGuiInterface() {
		if(!pCollection_) return;

		auto& slots = pCollection_->GetSlots();

		for(int i = 0; i < slots.size(); ++i) {
			auto& slot = slots[i];

			ImGui::PushID(i);
			ImGui::Checkbox("Enabled", &slot.enabled);
			ImGui::SameLine();
			ImGui::Text("%s", slot.name.c_str());
			ImGui::SameLine();
			if(ImGui::ArrowButton("Up", ImGuiDir_Up) && i > 0) {
				std::swap(slots[i], slots[i - 1]);
			}
			ImGui::SameLine();
			if(ImGui::ArrowButton("Down", ImGuiDir_Down) && i < slots.size() - 1) {
				std::swap(slots[i], slots[i + 1]);
			}

			const auto& effectNames = pCollection_->GetEffectNames();
			if(ImGui::BeginCombo("Effect Type", slot.name.c_str())) {
				for(int n = 0; n < effectNames.size(); ++n) {
					bool isSelected = (slot.name == effectNames[n]);
					if(ImGui::Selectable(effectNames[n].c_str(), isSelected)) {
						slot.name = effectNames[n];
						slot.pass = pCollection_->GetEffectByName(effectNames[n]); // pass 更新
					}
					if(isSelected) ImGui::SetItemDefaultFocus();
				}
				ImGui::EndCombo();
			}

			if(slot.pass) {
				ImGui::Indent();
				slot.pass->ShowImGui();
				ImGui::Unindent();
			}

			ImGui::Separator();
			ImGui::PopID();
		}
	}

	void PostProcessEditor::ApplyToGraph(PostEffectGraph* graph) {
		if(!graph || !pCollection_) return;

		// 名前とpassを再マッピング
		for(auto& slot : pCollection_->GetSlots()) {
			slot.pass = pCollection_->GetEffectByName(slot.name);
		}
		graph->SetPassesFromList(pCollection_->GetSlots());
	}
} // namespace CalyxEditor
