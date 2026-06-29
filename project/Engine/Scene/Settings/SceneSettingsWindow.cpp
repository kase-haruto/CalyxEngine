#include "SceneSettingsWindow.h"

#include <Engine/Collision/CollisionLayerTableUI.h>
#include <Engine/Scene/Context/SceneContext.h>
#include <externals/imgui/imgui.h>

namespace CalyxEngine {
	void SceneSettingsWindow::Open() {
		show_ = true;
	}

	void SceneSettingsWindow::Render(SceneContext* context) {
		if(!show_) {
			return;
		}

		ImGui::SetNextWindowSize(ImVec2(900.0f, 620.0f), ImGuiCond_FirstUseEver);
		if(!ImGui::Begin("Scene Settings", &show_)) {
			ImGui::End();
			return;
		}

		if(!context) {
			ImGui::TextDisabled("No active scene.");
			ImGui::End();
			return;
		}

		ImGui::Text("Scene: %s", context->GetSceneName().c_str());
		ImGui::TextDisabled("Changes are stored when the scene is saved.");
		ImGui::Separator();

		// 左側をカテゴリ選択、右側をカテゴリ固有UIとし、設定追加時の変更範囲を限定する。
		const float categoryWidth = 170.0f;
		ImGui::BeginChild("SceneSettingsCategories", ImVec2(categoryWidth, 0.0f), true);
		constexpr Category categories[] = {
			Category::Collision,
		};
		for(const Category category : categories) {
			const bool selected = selectedCategory_ == category;
			if(ImGui::Selectable(GetCategoryLabel(category), selected)) {
				selectedCategory_ = category;
			}
		}
		ImGui::EndChild();

		ImGui::SameLine();
		ImGui::BeginChild("SceneSettingsDetails", ImVec2(0.0f, 0.0f), false);
		ImGui::TextUnformatted(GetCategoryLabel(selectedCategory_));
		ImGui::Separator();
		DrawCategoryDetails(*context);
		ImGui::EndChild();

		ImGui::End();
	}

	const char* SceneSettingsWindow::GetCategoryLabel(Category category) {
		switch(category) {
		case Category::Collision:
			return "Collision";
		default:
			return "Unknown";
		}
	}

	void SceneSettingsWindow::DrawCategoryDetails(SceneContext& context) {
		switch(selectedCategory_) {
		case Category::Collision:
			// 現在シーンが所有する設定を明示的に渡し、別SceneContextへの誤編集を防止する。
			CollisionLayerTableUI::Draw(context.GetSettings().GetCollisionSettings());
			break;
		default:
			break;
		}
	}
}
