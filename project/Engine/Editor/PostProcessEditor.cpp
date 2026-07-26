#include "PostProcessEditor.h"

#include <Engine/PostProcess/Graph/PostEffectGraph.h>
#include <Engine/PostProcess/Slot/PostEffectSlot.h>
#include <Engine/PostProcess/Collection/PostProcessCollection.h>

#include <externals/imgui/imgui.h>
namespace CalyxEngine {
	PostProcessEditor::PostProcessEditor(const std::string& name)
		: BaseEditor(name) {}

	void PostProcessEditor::ShowImGuiInterface() {
		if(!pCollection_) return;

		// Collectionの並び順を直接編集し、表示順と実行Pass順を一致させる。
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
						// 種別名の変更と同時に実行実体を再解決し、名前とPassの不一致を残さない。
						slot.name = effectNames[n];
						slot.pass = pCollection_->GetEffectByName(effectNames[n]);
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

		// Asset再読込後の古いPass参照を、現在のCollectionが所有する実体へ再マッピングする。
		for(auto& slot : pCollection_->GetSlots()) {
			slot.pass = pCollection_->GetEffectByName(slot.name);
		}
		// Editorの順序・有効状態をGraphのRuntime実行列へ反映する。
		graph->SetPassesFromList(pCollection_->GetSlots());
	}
} // namespace CalyxEngine
