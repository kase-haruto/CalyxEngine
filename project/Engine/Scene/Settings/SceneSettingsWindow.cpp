#include "SceneSettingsWindow.h"

#include <Engine/Collision/CollisionLayerTableUI.h>
#include <Engine/Scene/Context/SceneContext.h>
#include <externals/imgui/imgui.h>

#include <array>
#include <algorithm>
#include <string>

namespace {
	void DrawSortingLayerSettings(SortingLayerSettings& settings) {
		static SortingLayerId selectedId = kDefaultSortingLayerId;
		static std::array<char, 64> newName{};
		static std::array<char, 64> renameName{};

		ImGui::TextUnformatted("Sorting Layers");
		ImGui::TextDisabled("Layers are drawn from top to bottom; lower entries appear in front.");
		ImGui::Separator();

		for(const auto& layer : settings.GetLayers()) {
			const bool selected = selectedId == layer.id;
			const std::string label = layer.name + "##SortingLayer" + std::to_string(layer.id);
			if(ImGui::Selectable(label.c_str(), selected)) {
				selectedId = layer.id;
				renameName.fill('\0');
				const size_t length = std::min(layer.name.size(), renameName.size() - 1);
				std::copy_n(layer.name.data(), length, renameName.data());
			}
		}

		ImGui::Separator();
		ImGui::InputText("New Layer", newName.data(), newName.size());
		ImGui::SameLine();
		if(ImGui::Button("Add") && newName[0] != '\0') {
			SortingLayerId addedId = kDefaultSortingLayerId;
			if(settings.AddLayer(newName.data(), &addedId)) {
				selectedId = addedId;
				newName.fill('\0');
			}
		}

		const SortingLayer* selected = settings.FindLayer(selectedId);
		if(!selected) {
			selectedId = kDefaultSortingLayerId;
			selected = settings.FindLayer(selectedId);
		}
		if(!selected) return;

		ImGui::Separator();
		ImGui::Text("Selected ID: %u", static_cast<unsigned>(selected->id));
		const bool isDefault = selected->id == kDefaultSortingLayerId;
		ImGui::BeginDisabled(isDefault);
		ImGui::InputText("Name", renameName.data(), renameName.size());
		if(ImGui::Button("Rename") && renameName[0] != '\0') {
			settings.RenameLayer(selectedId, renameName.data());
		}
		ImGui::SameLine();
		if(ImGui::Button("Delete")) {
			settings.RemoveLayer(selectedId);
			selectedId = kDefaultSortingLayerId;
		}
		ImGui::EndDisabled();

		if(ImGui::Button("Move Back")) settings.MoveLayer(selectedId, -1);
		ImGui::SameLine();
		if(ImGui::Button("Move Front")) settings.MoveLayer(selectedId, 1);
	}
}

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
			Category::Rendering,
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
		case Category::Rendering:
			return "Rendering";
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
		case Category::Rendering:
			DrawSortingLayerSettings(context.GetSettings().GetSortingLayerSettings());
			break;
		default:
			break;
		}
	}
}
