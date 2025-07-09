#include "PlaceToolPanel.h"

// engine
#include <Engine/Assets/Texture/TextureManager.h>
#include <Engine/Objects/3D/Actor/BaseGameObject.h>
#include <Engine/Application/Effects/Particle/Object/ParticleSystemObject.h>
#include <Engine/Scene/Context/SceneContext.h>
#include <Engine/System/Command/EditorCommand/LevelEditorCommand/CreateObjectCommand/CreateShapeObjectCommand.h>
#include <Engine/System/Command/EditorCommand/LevelEditorCommand/CreateObjectCommand/CreateParticleSystemCommand.h>
#include <Engine/System/Command/Manager/CommandManager.h>
#include <Engine/Scene/Utirity/SceneUtility.h>

// externals
#include <externals/imgui/imgui.h>

PlaceToolPanel::PlaceToolPanel() : IEngineUI("PlaceToolPanel"){
	RegisterPlaceItems();
}

void PlaceToolPanel::RegisterPlaceItems(){
	auto& shapeItems = categoryItems_[PlaceItemCategory::Shape];
	const std::vector<std::pair<ShapeObjType, std::string>> shapes = {
		{ShapeObjType::Plane, "plane"},
		{ShapeObjType::Cube, "cube"},
		{ShapeObjType::Sphere, "sphere"},
		{ShapeObjType::Cylinder, "cylinder"},
		{ShapeObjType::Cone, "cone"},
		{ShapeObjType::Torus, "torus"}
	};
	for (auto& [type, name] : shapes){
		std::string objName = name;
		std::string modelName = (name == "cube" ? "debugCube.obj" :
								 name == "sphere" ? "debugSphere.obj" :
								 name + ".obj");
		shapeItems.push_back({
			PlaceItemCategory::Shape,
			name,
			TextureManager::GetInstance()->LoadTexture("UI/Tool/" + name + ".png"),
			{64, 64},
			[this, objName, modelName] (){
				auto factory = [objName, modelName] (){
					auto obj = std::make_unique<BaseGameObject>(modelName, objName);
					obj->Initialize();
					return obj;
				};
				CommandManager::GetInstance()->Execute(
					std::make_unique<CreateShapeObjectCommand>(pSceneContext_, factory));
			}
							 });
	}

	auto& particleItems = categoryItems_[PlaceItemCategory::Particle];
	particleItems.push_back({
		PlaceItemCategory::Particle,
		"Particle",
		TextureManager::GetInstance()->LoadTexture("UI/Tool/particle.png"),
		{64, 64},
		[this] (){
			std::string name = "ParticleSystem";
			auto factory = [name] (){
				auto obj = std::make_unique<ParticleSystemObject>(name);
				obj->Initialize();
				return obj;
			};
			CommandManager::GetInstance()->Execute(
				std::make_unique<CreateParticleSystemObjectCommand>(pSceneContext_, factory, name));
		}
							});
}

void PlaceToolPanel::Render(){
	ImGui::Begin(panelName_.c_str());
	if (!pSceneContext_){
		ImGui::Text("sceneContext not set");
		ImGui::End();
		return;
	}
	RenderCategoryItems();
	ImGui::End();
}

void PlaceToolPanel::RenderCategoryItems() {
	constexpr int columns = 4; // 1カテゴリあたりの列数

	for (auto& [category, items] : categoryItems_) {

		// カテゴリ名をわかりやすく
		const char* categoryName = "";
		switch (category) {
			case PlaceItemCategory::Shape:    categoryName = "Shape"; break;
			case PlaceItemCategory::Particle: categoryName = "Particle"; break;
			case PlaceItemCategory::Model:    categoryName = "Model"; break;
			default:                          categoryName = "Unknown"; break;
		}

		// UE風に: カテゴリを折りたたみ
		if (ImGui::CollapsingHeader(categoryName, ImGuiTreeNodeFlags_DefaultOpen)) {

			ImGui::Columns(columns, nullptr, false);

			for (size_t i = 0; i < items.size(); ++i) {
				const auto& item = items[i];

				ImGui::PushID(static_cast<int>(i));

				if (ImGui::ImageButton((ImTextureID)item.texture.ptr, ImVec2(item.iconSize.x, item.iconSize.y))) {
					item.createFunc();
				}

				if (ImGui::IsItemHovered()) {
					ImGui::SetTooltip("%s", item.name.c_str());
				}

				ImGui::TextWrapped("%s", item.name.c_str());

				ImGui::NextColumn();

				ImGui::PopID();
			}

			ImGui::Columns(1);
			ImGui::Spacing();
		}
	}
}

void PlaceToolPanel::OnSceneContextChanged(SceneContext* newContext){
	pSceneContext_ = newContext;
}
