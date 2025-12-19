#include "PlaceToolPanel.h"

// --- engine -----------------------------------------------------------------
#include <Engine/Assets/Texture/TextureManager.h>
#include <Engine/Scene/Utility/SceneUtility.h>
#include <Engine/Scene/Context/SceneContext.h>
#include <Engine/System/Command/Manager/CommandManager.h>
#include <Engine/System/Command/EditorCommand/LevelEditorCommand/CreateObjectCommand/CreateObjectCommand.h>
#include <Engine/Application/Effects/Particle/Object/ParticleSystemObject.h>
#include <Engine/Objects/3D/Actor/BaseGameObject.h>
#include <Engine/Application/Effects/FxObject.h>

// --- game objects -----------------------------------------------------------
#include <Game/3dObject/Actor/Enemy/Spawner/EnemySpawner.h>
#include <Game/3dObject/Actor/Boss/Spawner/BossSpawner.h>

//event
#include <Game/Event/Camera/CameraTurnAroundEvent.h>
#include <Game/Event/Spawn/EnemySpawnEvent.h>

// --- externals --------------------------------------------------------------
#include <externals/imgui/imgui.h>

namespace CalyxEditor {
	// ============================================================================
	//  ctor
	// ============================================================================
	PlaceToolPanel::PlaceToolPanel()
		: IEngineUI("PlaceToolPanel") { RegisterPlaceItems(); }

	// ============================================================================
	//  アイテム登録
	// ============================================================================
	void PlaceToolPanel::RegisterPlaceItems() {
		// ----------------------------- Shape ------------------------------------
		auto&													shapeItems = categoryItems_[PlaceItemCategory::Shape];
		const std::vector<std::pair<ShapeObjType, std::string>> shapes	   = {
			{ShapeObjType::Plane, "plane"},
			{ShapeObjType::Cube, "cube"},
			{ShapeObjType::Sphere, "sphere"},
			{ShapeObjType::Cylinder, "cylinder"},
			{ShapeObjType::Cone, "cone"},
			{ShapeObjType::Torus, "torus"}};

		for(auto& [type, name] : shapes) {
			std::string objName	  = name;
			std::string modelName = (name == "cube"
										 ? "debugCube.obj"
									 : name == "sphere"
										 ? "debugSphere.obj"
										 : name + ".obj");

			shapeItems.push_back({PlaceItemCategory::Shape,
								  objName,
								  TextureManager::GetInstance()->LoadTexture("UI/Tool/" + name + ".png"),
								  {64, 64},
								  [modelName, objName]() {
									  auto factory = [modelName, objName]() {
										  auto obj = SceneAPI::Instantiate<BaseGameObject>(modelName, objName);
										  obj->Initialize();
										  obj->GetCollider()->SetCollisionEnabled(false);
										  return obj;
									  };
									  CommandManager::GetInstance()->Execute(
										  std::make_unique<CreateObjectCommand<BaseGameObject>>(
											  SceneContext::Current(), factory, "Create Shape"));
								  }});
		}

		// ---------------------------- Particle ----------------------------------
		{
			auto& particleItems = categoryItems_[PlaceItemCategory::Particle];
			particleItems.push_back({PlaceItemCategory::Particle,
									 "ParticleSystem",
									 TextureManager::GetInstance()->LoadTexture("UI/Tool/particle.png"),
									 {64, 64},
									 []() {
										 const std::string name	   = "ParticleSystem";
										 auto			   factory = [name]() {
											  auto obj = SceneAPI::Instantiate<CalyxEffect::ParticleSystemObject>(name);
											  obj->Initialize();
											  return obj;
										 };
										 CommandManager::GetInstance()->Execute(
											 std::make_unique<CreateObjectCommand<CalyxEffect::ParticleSystemObject>>(
												 SceneContext::Current(), factory, "Create ParticleSystem"));
									 }});
		}

		{
			auto& particleItems = categoryItems_[PlaceItemCategory::Particle];
			particleItems.push_back({PlaceItemCategory::Particle,
									 "EffectObject",
									 TextureManager::GetInstance()->LoadTexture("UI/Tool/particle.png"),
									 {64, 64},
									 []() {
										 const std::string name	   = "EffectObject";
										 auto			   factory = [name]() {
											  auto obj = SceneAPI::Instantiate<CalyxEffect::FxObject>(name);
											  obj->Initialize();
											  return obj;
										 };
										 CommandManager::GetInstance()->Execute(
											 std::make_unique<CreateObjectCommand<CalyxEffect::FxObject>>(
												 SceneContext::Current(), factory, "Create EffectObject"));
									 }});
		}

		// ---------------------------- Spawner -----------------------------------
		{
			auto& spawnerItems = categoryItems_[PlaceItemCategory::InGameObject];
			spawnerItems.push_back({PlaceItemCategory::InGameObject,
									"EnemySpawner",
									{},
									{64, 64},
									[]() {
										auto* ctx	  = SceneContext::Current();
										auto  factory = []() {
											 auto obj = SceneAPI::Instantiate<EnemySpawner>("EnemySpawner");
											 obj->ApplyConfig();
											 obj->Initialize();
											 return obj;
										};
										CommandManager::GetInstance()->Execute(
											std::make_unique<CreateObjectCommand<EnemySpawner>>(
												ctx, factory, "Create EnemySpawner"));
									}});
		}

		{
			auto& spawnerItems = categoryItems_[PlaceItemCategory::InGameObject];
			spawnerItems.push_back({PlaceItemCategory::InGameObject,
									"BossSpawner",
									{},
									{64, 64},
									[]() {
										auto* ctx = SceneContext::Current();

										// 既にシーンに BossSpawner があるなら作成しない
										if(ctx->FindFirst<BossSpawner>()) {
											OutputDebugStringA("[PlaceTool] BossSpawner is limited to one per scene.\n");
											return;
										}

										auto factory = []() {
											auto obj = SceneAPI::Instantiate<BossSpawner>("BossSpawner");
											obj->ApplyConfig();
											obj->Initialize();
											return obj;
										};

										CommandManager::GetInstance()->Execute(
											std::make_unique<CreateObjectCommand<BossSpawner>>(
												ctx, factory, "Create BossSpawner"));
									}});
		}

		// ---------------------------- Events -----------------------------------
		{
			auto& eventItems = categoryItems_[PlaceItemCategory::Event];
			eventItems.push_back({PlaceItemCategory::Event,
								  "CameraTurnAroundEvent",
								  {},
								  {64, 64},
								  [] {
									  auto* ctx = SceneContext::Current();

									  auto factory = []() {
										  auto obj = SceneAPI::Instantiate<CameraTurnAroundEvent>("CameraTurnAroundEvent");
										  // obj->ApplyConfig();
										  return obj;
									  };

									  CommandManager::GetInstance()->Execute(
										  std::make_unique<CreateObjectCommand<CameraTurnAroundEvent>>(
											  ctx, factory, "Create CameraTurnAroundEvent"));
								  }});
		}

		{
			auto& eventItems = categoryItems_[PlaceItemCategory::Event];
			eventItems.push_back({PlaceItemCategory::Event,
								  "EnemySpawnEvent",
								  {},
								  {64, 64},
								  [] {
									  auto* ctx = SceneContext::Current();

									  auto factory = []() {
										  auto obj = SceneAPI::Instantiate<EnemySpawnEvent>("EnemySpawnEvent");
										  // obj->ApplyConfig();
										  return obj;
									  };

									  CommandManager::GetInstance()->Execute(
										  std::make_unique<CreateObjectCommand<EnemySpawnEvent>>(
											  ctx, factory, "Create CameraTurnAroundEvent"));
								  }});
		}

		// ---------------------------- Models ------------------------------------
		auto&						   modelItems = categoryItems_[PlaceItemCategory::Model];
		const std::vector<std::string> modelNames = {
			"largeBuilding.obj",
			"tallBuilding_01.obj",
			"tallBuilding_02.obj",
			"smallBuilding_01.obj"};

		for(const auto& modelName : modelNames) {
			std::string displayName = modelName;
			if(const size_t dot = modelName.find('.'); dot != std::string::npos) {
				displayName = modelName.substr(0, dot); // 拡張子を除いた名前を表示に使う
			}

			modelItems.push_back({PlaceItemCategory::Model,
								  displayName,
								  {},
								  {64, 64},
								  [modelName, displayName]() {
									  auto factory = [modelName, displayName]() {
										  auto obj = SceneAPI::Instantiate<BaseGameObject>(modelName, displayName);
										  obj->Initialize();
										  obj->GetCollider()->SetCollisionEnabled(false);
										  return obj;
									  };
									  CommandManager::GetInstance()->Execute(
										  std::make_unique<CreateObjectCommand<BaseGameObject>>(
											  SceneContext::Current(), factory, "Create Model"));
								  }});
		}
	}

	// ============================================================================
	//  パネル描画
	// ============================================================================
	void PlaceToolPanel::Render() {
		if(!IsShow()) return; // 必要ないなら早期りたーん

		bool open = true;
		ImGui::Begin(panelName_.c_str(), &open);

		if(!SceneContext::Current()) {
			ImGui::Text("sceneContext not set");
			ImGui::End();
			if(!open) SetShow(false);
			return;
		}

		RenderCategoryItems();
		ImGui::End();
		if(!open) SetShow(false);
	}

	// ============================================================================
	//  カテゴリ別アイテム描画
	// ============================================================================
	void PlaceToolPanel::RenderCategoryItems() {
		constexpr int columns = 4; // 1 カテゴリあたりの列数

		for(auto& [category, items] : categoryItems_) {

			// カテゴリ名
			const char* categoryName = "";
			switch(category) {
			case PlaceItemCategory::Shape:
				categoryName = "Shape";
				break;
			case PlaceItemCategory::Particle:
				categoryName = "Particle";
				break;
			case PlaceItemCategory::InGameObject:
				categoryName = "IngameObject";
				break;
			case PlaceItemCategory::Event:
				categoryName = "Event";
				break;
			case PlaceItemCategory::Model:
				categoryName = "Model";
				break;
			default:
				categoryName = "Unknown";
				break;
			}

			// 折りたたみヘッダ
			if(ImGui::CollapsingHeader(categoryName)) {
				ImGui::Columns(columns, nullptr, false);

				for(size_t i = 0; i < items.size(); ++i) {
					const auto& item = items[i];
					ImGui::PushID(static_cast<int>(i));

					// アイコン有無で描画を切り替え
					bool clicked = false;
					if(item.texture.ptr) {
						clicked = ImGui::ImageButton(
							(ImTextureID)item.texture.ptr,
							ImVec2(item.iconSize.x, item.iconSize.y));
					} else {
						clicked = ImGui::Button(
							item.name.c_str(),
							ImVec2(item.iconSize.x, item.iconSize.y));
					}

					if(clicked) {
						item.createFunc();
					}

					if(ImGui::IsItemHovered()) {
						ImGui::SetTooltip("%s", item.name.c_str());
					}

					// ラベル
					ImGui::TextWrapped("%s", item.name.c_str());

					ImGui::NextColumn();
					ImGui::PopID();
				}

				ImGui::Columns(1);
				ImGui::Spacing();
			}
		}
	}
}

