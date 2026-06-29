#include "CollisionLayerTableUI.h"

#include <Engine/Collision/CollisionLayerSettings.h>
#include <externals/imgui/imgui.h>

#include <array>
#include <cstdio>

namespace CollisionLayerTableUI {
	void Draw() {
		// 既存呼び出しは現在SceneContextが有効化した設定へ委譲する。
		Draw(*CollisionLayerSettings::GetInstance());
	}

	void Draw(CollisionLayerSettings& sceneSettings) {
		// Scene Settingsから渡された設定を直接編集し、別シーンの設定へ書き込まない。
		auto* settings = &sceneSettings;

		// ImGuiは毎フレームDrawを呼ぶImmediate Mode UIなので、入力途中の状態だけをstaticで保持する。
		// Layer本体の正しい状態は常にCollisionLayerSettings側にあり、UI側へ複製しない。
		static CollisionLayerId selectedLayerId = kDefaultCollisionLayerId;
		static CollisionLayerId renameBufferLayerId = static_cast<CollisionLayerId>(0xff);
		static std::array<char, 64> addName{};
		static std::array<char, 64> renameName{};
		static const CollisionLayerSettings* displayedSettings = nullptr;

		// Scene切替後に同じ数値IDを別Layerとして誤選択しないよう、UIだけの選択状態を初期化する。
		if(displayedSettings != settings) {
			displayedSettings = settings;
			selectedLayerId = kDefaultCollisionLayerId;
			renameBufferLayerId = static_cast<CollisionLayerId>(0xff);
			addName[0] = '\0';
			renameName[0] = '\0';
		}

		// Layer一覧はSettingsがID昇順に維持しているため、そのまま安定した順番で表示できる。
		ImGui::TextUnformatted("Collision Layers");
		ImGui::Separator();
		for(const auto& layer : settings->GetLayers()) {
			ImGui::PushID(static_cast<int>(layer.id));
			const bool selected = selectedLayerId == layer.id;
			if(ImGui::Selectable(layer.name.c_str(), selected)) {
				selectedLayerId = layer.id;
			}
			ImGui::PopID();
		}

		ImGui::InputText("New Layer", addName.data(), addName.size());
		const bool reachedLayerLimit = settings->GetLayers().size() >= kMaxCollisionLayerCount;
		// 空名または32Layer到達時は、失敗する操作を事前に無効化する。
		ImGui::BeginDisabled(reachedLayerLimit || addName[0] == '\0');
		if(ImGui::Button("Add Layer")) {
			CollisionLayerId addedLayerId = kDefaultCollisionLayerId;
			if(settings->AddLayer(addName.data(), &addedLayerId)) {
				selectedLayerId = addedLayerId;
				addName[0] = '\0';
			}
		}
		ImGui::EndDisabled();

		if(!settings->IsValidLayerId(selectedLayerId)) {
			// 外部処理で選択中Layerが削除されても、次の描画でDefaultへ戻して参照切れを防ぐ。
			selectedLayerId = kDefaultCollisionLayerId;
		}
		if(renameBufferLayerId != selectedLayerId) {
			// 選択が切り替わった時だけ入力バッファを同期し、入力中の文字列を毎フレーム上書きしない。
			renameBufferLayerId = selectedLayerId;
			std::snprintf(renameName.data(), renameName.size(), "%s", settings->GetLayerName(selectedLayerId).c_str());
		}

		const bool isDefaultLayer = selectedLayerId == kDefaultCollisionLayerId;
		// Defaultの変更禁止をSettingsだけでなくUIでも明示し、無効操作をユーザーへ提示しない。
		ImGui::BeginDisabled(isDefaultLayer);
		ImGui::InputText("Layer Name", renameName.data(), renameName.size());
		if(ImGui::Button("Rename Layer") && settings->RenameLayer(selectedLayerId, renameName.data())) {
			renameBufferLayerId = static_cast<CollisionLayerId>(0xff);
		}
		ImGui::SameLine();
		if(ImGui::Button("Delete Layer") && settings->RemoveLayer(selectedLayerId)) {
			selectedLayerId = kDefaultCollisionLayerId;
			renameBufferLayerId = static_cast<CollisionLayerId>(0xff);
		}
		ImGui::EndDisabled();
		if(isDefaultLayer) {
			ImGui::TextDisabled("Default Layer cannot be renamed or deleted.");
		}

		ImGui::Spacing();
		ImGui::TextUnformatted("Collision Matrix");
		ImGui::Separator();

		const auto& layers = settings->GetLayers();
		// 左端の行見出しに1列、各LayerのチェックボックスにLayer数分の列を割り当てる。
		const int columnCount = static_cast<int>(layers.size()) + 1;
		if(ImGui::BeginTable("CollisionLayerMatrix", columnCount,
			ImGuiTableFlags_Borders | ImGuiTableFlags_ScrollX | ImGuiTableFlags_SizingFixedFit)) {
			ImGui::TableSetupColumn("Layer");
			for(const auto& layer : layers) {
				ImGui::TableSetupColumn(layer.name.c_str());
			}
			ImGui::TableHeadersRow();

			for(const auto& rowLayer : layers) {
				// 1行がMatrixのLayer A、各列がLayer Bに対応する。
				ImGui::TableNextRow();
				ImGui::TableSetColumnIndex(0);
				ImGui::TextUnformatted(rowLayer.name.c_str());

				for(size_t column = 0; column < layers.size(); ++column) {
					const auto& columnLayer = layers[column];
					ImGui::TableSetColumnIndex(static_cast<int>(column) + 1);
					// 全セルの表示ラベルは同じなので、行IDと列IDをImGui IDスタックへ積んで一意化する。
					ImGui::PushID(static_cast<int>(rowLayer.id));
					ImGui::PushID(static_cast<int>(columnLayer.id));
					bool canCollide = settings->GetMatrix().CanCollide(rowLayer.id, columnLayer.id);
					if(ImGui::Checkbox("##CanCollide", &canCollide)) {
						// SetCanCollide が反対側も同時更新するため、UI 操作後も Matrix は常に対称になる。
						settings->GetMatrix().SetCanCollide(rowLayer.id, columnLayer.id, canCollide);
					}
					ImGui::PopID();
					ImGui::PopID();
				}
			}
			ImGui::EndTable();
		}
	}
}
