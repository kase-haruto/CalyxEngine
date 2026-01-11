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
		if(!IsShow()) return;

		bool open = true;
		// 少しパディングを入れて見やすく
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8.0f, 8.0f));
		if(ImGui::Begin(panelName_.c_str(), &open)) {
			if(!SceneContext::Current()) {
				ImGui::Text("sceneContext not set");
			} else {
				// 2カラムのテーブルを作成 (Sidebar | Content)
				// Resizable: 境界線をドラッグ可能
				// BordersInnerV: 縦の境界線を表示
				if(ImGui::BeginTable("PlaceToolTable", 2, ImGuiTableFlags_Resizable | ImGuiTableFlags_BordersInnerV)) {
					// --- Sidebar ---
					ImGui::TableSetupColumn("Category", ImGuiTableColumnFlags_WidthFixed, 100.0f);
					ImGui::TableSetupColumn("Items", ImGuiTableColumnFlags_None);
					
					// Sidebar 描画
					ImGui::TableNextColumn();
					RenderSidebar();

					// Content 描画
					ImGui::TableNextColumn();
					RenderContent();

					ImGui::EndTable();
				}
			}
		}
		ImGui::End();
		ImGui::PopStyleVar();

		if(!open) SetShow(false);
	}

	// ============================================================================
	//  サイドバー（カテゴリ一覧）
	// ============================================================================
	void PlaceToolPanel::RenderSidebar() {
		// カテゴリ名の定義マップ
		static const std::vector<std::pair<PlaceItemCategory, std::string>> categoryNames = {
			{PlaceItemCategory::Shape, "Shapes"},
			{PlaceItemCategory::Light, "Lights"},
			{PlaceItemCategory::Particle, "Particles"},
			{PlaceItemCategory::InGameObject, "Game Objects"},
			{PlaceItemCategory::Model, "Models"},
			{PlaceItemCategory::Event, "Events"},
		};

		ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.2f, 0.2f, 0.2f, 1.0f)); // 選択時の色を少し調整
		for(const auto& [cat, name] : categoryNames) {
			// カテゴリにアイテムが含まれているかチェック (空なら表示しない等の制御が必要ならここ)
			// 今回は全部表示する方針で
			
			bool isSelected = (selectedCategory_ == cat);
			if(ImGui::Selectable(name.c_str(), isSelected)) {
				selectedCategory_ = cat;
				// カテゴリ切り替え時にフィルタリセットするかはお好みで。今回は維持する。
			}
		}
		ImGui::PopStyleColor();
	}

	// ============================================================================
	//  コンテンツ（検索 + アイテムグリッド）
	// ============================================================================
	void PlaceToolPanel::RenderContent() {
		// --- Search Bar ---
		ImGui::SetNextItemWidth(-1.0f); // 横幅いっぱい
		filter_.Draw("##Search", -1.0f);
		
		ImGui::Separator();
		ImGui::Spacing();

		// --- Item Grid ---
		// 子ウィンドウにしてスクロール可能にする
		if(ImGui::BeginChild("ItemGrid", ImVec2(0, 0), false)) {
			
			// グリッドのカラム数はウィンドウ幅に応じて動的に決めるのが望ましいが、
			// とりあえず固定サイズまたはImGui::Columnsを使う
			float panelWidth = ImGui::GetContentRegionAvail().x;
			float itemWidth  = 80.0f;  // アイコンサイズ(64) + パディング
			int   columns    = static_cast<int>(panelWidth / itemWidth);
			if(columns < 1) columns = 1;

			if(ImGui::BeginTable("Grid", columns)) {
				
				auto drawItem = [](const PlaceItem& item) {
					ImGui::PushID(&item);
					
					// グループ化して全体をクリッカブルっぽく見せる
					ImGui::BeginGroup();
					
					// アイコン（画像がなければボタン）
					bool clicked = false;
					float iconSize = 64.0f; 
					
					// 中央寄せのためのカーソル操作
					float availW = ImGui::GetContentRegionAvail().x;
					float offX = (availW - iconSize) * 0.5f;
					if (offX > 0.0f) ImGui::SetCursorPosX(ImGui::GetCursorPosX() + offX);

					if(item.texture.ptr) {
						clicked = ImGui::ImageButton(
							(ImTextureID)item.texture.ptr,
							ImVec2(iconSize, iconSize));
					} else {
						clicked = ImGui::Button(item.name.c_str(), ImVec2(iconSize, iconSize));
					}

					// テキストも中央寄せ
					{
						float textW = ImGui::CalcTextSize(item.name.c_str()).x;
						float textOffX = (availW - textW) * 0.5f;
						if (textOffX > 0.0f) ImGui::SetCursorPosX(ImGui::GetCursorPosX() + textOffX);
						ImGui::TextWrapped("%s", item.name.c_str());
					}

					ImGui::EndGroup();

					if(clicked) {
						item.createFunc();
					}
					
					ImGui::PopID();
				};

				// フィルタが有効な場合は全カテゴリから検索
				if (filter_.IsActive()) {
					for (const auto& [cat, items] : categoryItems_) {
						for (const auto& item : items) {
							if (filter_.PassFilter(item.name.c_str())) {
								ImGui::TableNextColumn();
								drawItem(item);
							}
						}
					}
				} 
				else {
					// 選択中のカテゴリのみ表示
					if (categoryItems_.count(selectedCategory_)) {
						const auto& items = categoryItems_[selectedCategory_];
						for (const auto& item : items) {
							ImGui::TableNextColumn();
							drawItem(item);
						}
					}
				}

				ImGui::EndTable();
			}
		}
		ImGui::EndChild();
	}
}

