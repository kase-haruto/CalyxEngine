#include "PostProcessEditor.h"

#include <Engine/PostProcess/Graph/PostEffectGraph.h>
#include <Engine/PostProcess/Slot/PostEffectSlot.h>
#include <Engine/PostProcess/Collection/PostProcessCollection.h>

#include <externals/imgui/imgui.h>


PostProcessEditor::PostProcessEditor(const std::string& name) :BaseEditor(name){}

void PostProcessEditor::ShowImGuiInterface(){
	// スロットの一覧を表示
	for (int i = 0; i < slots_.size(); ++i){
		auto& slot = slots_[i];

		ImGui::PushID(i); // 同じUIが複数ある場合の識別

		// 有効化切り替え
		ImGui::Checkbox("Enabled", &slot.enabled);

		ImGui::SameLine();

		// スロット名表示
		ImGui::Text("%s", slot.name.c_str());

		// 上下入れ替えボタン
		ImGui::SameLine();
		if (ImGui::ArrowButton("Up", ImGuiDir_Up) && i > 0){
			std::swap(slots_[i], slots_[i - 1]);
		}
		ImGui::SameLine();
		if (ImGui::ArrowButton("Down", ImGuiDir_Down) && i < slots_.size() - 1){
			std::swap(slots_[i], slots_[i + 1]);
		}

		static int selectedEffect = 0; // 選択中のインデックス（例）
		const auto& effectNames = pCollection_->GetEffectNames();

		if (ImGui::BeginCombo("Effect Type", slot.name.c_str())){
			for (int n = 0; n < effectNames.size(); ++n){
				bool isSelected = (slot.name == effectNames[n]);
				if (ImGui::Selectable(effectNames[n].c_str(), isSelected)){
					slot.name = effectNames[n];
				}
				if (isSelected)
					ImGui::SetItemDefaultFocus();
			}
			ImGui::EndCombo();
		}

		ImGui::Separator();
		ImGui::PopID();
	}

}

void PostProcessEditor::SetPostEffectCollection(PostProcessCollection* postProcessCollection){
	pCollection_ = postProcessCollection;
	if (pCollection_){
		slots_.clear();
		for (const auto& effectName : pCollection_->GetEffectNames()){
			slots_.emplace_back(PostEffectSlot {effectName, false, pCollection_->GetEffectByName(effectName)});
		}
	}
}

void PostProcessEditor::ApplyToGraph(PostEffectGraph* graph){
	if (!graph || !pCollection_) return;

	// slots_ の pass を更新する
	for (auto& slot : slots_){
		// スロット名に対応するエフェクトパスを取得
		slot.pass = pCollection_->GetEffectByName(slot.name);
	}

	// Graph に反映
	graph->SetPassesFromList(slots_);
}

