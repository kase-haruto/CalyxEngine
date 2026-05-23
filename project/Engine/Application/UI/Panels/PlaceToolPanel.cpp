#include "PlaceToolPanel.h"

// --- engine -----------------------------------------------------------------
#include <Engine/Application/Effects/FxObject.h>
#include <Engine/Application/Effects/Particle/Object/ParticleSystemObject.h>
#include <Engine/Assets/Texture/TextureManager.h>
#include <Engine/Objects/3D/Actor/BaseGameObject.h>
#include <Engine/Objects/3D/Actor/Registry/SceneObjectRegistry.h>
#include <Engine/Objects/3D/Actor/SplineDeformObject.h>
#include <Engine/Scene/Context/SceneContext.h>
#include <Engine/Scene/Utility/SceneUtility.h>
#include <Engine/System/Command/EditorCommand/LevelEditorCommand/CreateObjectCommand/CreateObjectCommand.h>
#include <Engine/System/Command/Manager/CommandManager.h>

// --- game objects -----------------------------------------------------------


// event

// --- externals --------------------------------------------------------------
#include "Engine/Assets/Manager/AssetManager.h"

#include <externals/imgui/imgui.h>

namespace CalyxEngine {
	namespace {
		D3D12_GPU_DESCRIPTOR_HANDLE LoadPlaceIcon(const std::string& iconPath) {
			if(iconPath.empty()) {
				return {};
			}
			return AssetManager::GetInstance()->GetTextureManager()->LoadTexture(iconPath);
		}

		std::shared_ptr<SceneObject> CreateRegisteredObjectForScene(const std::string&		   typeName,
																	 const std::string&		   displayName,
																	 ObjectType				   objectType,
																	 const CalyxEngine::Vector3& pos,
																	 bool					   transient) {
			auto* ctx = SceneContext::Current();
			if(!ctx) {
				return nullptr;
			}

			auto obj = SceneObjectRegistry::Get().Create(typeName);
			if(!obj) {
				return nullptr;
			}

			obj->SetName(displayName.empty() ? typeName : displayName, objectType);
			obj->SetTransient(transient);
			ctx->AddObject(obj);
			obj->Initialize();
			obj->GetWorldTransform().translation = pos;
			return obj;
		}
	} // namespace

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
			{ShapeObjType::Cylinder, "Cylinder"},
			{ShapeObjType::Cone, "cone"},
			{ShapeObjType::Torus, "Torus"}};

		for(auto& [type, name] : shapes) {
			std::string objName	  = name;
			std::string modelName = (name == "cube"
										 ? "debugCube.obj"
									 : name == "sphere"
										 ? "debugSphere.obj"
										 : name + ".obj");

			shapeItems.push_back({PlaceItemCategory::Shape,
								  objName,
								  AssetManager::GetInstance()->GetTextureManager()->LoadTexture("UI/Tool/" + name + ".dds"),
								  {64, 64},
								  [modelName, objName](const CalyxEngine::Vector3& pos) {
									  auto factory = [modelName, objName, pos]() {
										  auto obj = SceneAPI::Instantiate<BaseGameObject>(modelName, objName);
										  obj->Initialize();
										  obj->GetCollider()->SetCollisionEnabled(false);
										  obj->GetWorldTransform().translation = pos;
										  return obj;
									  };
									  CommandManager::GetInstance()->Execute(
										  std::make_unique<CreateObjectCommand<BaseGameObject>>(
											  SceneContext::Current(), factory, "Create Shape"));
								  },
								  [modelName, objName]() {
									  auto obj = SceneAPI::Instantiate<BaseGameObject>(modelName, objName);
									  obj->Initialize();
									  obj->GetCollider()->SetCollisionEnabled(false);
									  obj->SetTransient(true);
									  return obj;
								  }});
		}

		shapeItems.push_back({PlaceItemCategory::Shape,
							  "Spline Wall Deform",
							  AssetManager::GetInstance()->GetTextureManager()->LoadTexture("UI/Tool/cylinder.dds"),
							  {64, 64},
							  []([[maybe_unused]] const CalyxEngine::Vector3& pos) {
								  auto factory = []() {
									  auto obj = SceneAPI::Instantiate<SplineDeformObject>();
									  obj->Initialize();
									  return obj;
								  };
								  CommandManager::GetInstance()->Execute(
									  std::make_unique<CreateObjectCommand<SplineDeformObject>>(
										  SceneContext::Current(), factory, "Create Spline Wall Deform"));
							  },
							  []() {
								  auto obj = SceneAPI::Instantiate<SplineDeformObject>();
								  obj->Initialize();
								  obj->SetTransient(true);
								  return obj;
							  }});

		// ---------------------------- Particle ----------------------------------
		{
			auto& particleItems = categoryItems_[PlaceItemCategory::Particle];
			particleItems.push_back({PlaceItemCategory::Particle,
									 "ParticleSystem",
									 AssetManager::GetInstance()->GetTextureManager()->LoadTexture("UI/Tool/particle.dds"),
									 {64, 64},
									 [](const CalyxEngine::Vector3& pos) {
										 const std::string name	   = "ParticleSystem";
										 auto			   factory = [name, pos]() {
											  auto obj = SceneAPI::Instantiate<CalyxEngine::ParticleSystemObject>(name);
											  obj->Initialize();
											  obj->GetWorldTransform().translation = pos;
											  return obj;
										 };
										 CommandManager::GetInstance()->Execute(
											 std::make_unique<CreateObjectCommand<CalyxEngine::ParticleSystemObject>>(
												 SceneContext::Current(), factory, "Create ParticleSystem"));
									 },
									 []() {
										 auto obj = SceneAPI::Instantiate<CalyxEngine::ParticleSystemObject>("ParticleSystem");
										 obj->Initialize();
										 obj->SetTransient(true);
										 return obj;
									 }});
		}

		{
			auto& particleItems = categoryItems_[PlaceItemCategory::Particle];
			particleItems.push_back({PlaceItemCategory::Particle,
									 "EffectObject",
									 AssetManager::GetInstance()->GetTextureManager()->LoadTexture("UI/Tool/particle.dds"),
									 {64, 64},
									 [](const CalyxEngine::Vector3& pos) {
										 const std::string name	   = "EffectObject";
										 auto			   factory = [name, pos]() {
											  auto obj = SceneAPI::Instantiate<CalyxEngine::FxObject>(name);
											  obj->Initialize();
											  obj->GetWorldTransform().translation = pos;
											  return obj;
										 };
										 CommandManager::GetInstance()->Execute(
											 std::make_unique<CreateObjectCommand<CalyxEngine::FxObject>>(
												 SceneContext::Current(), factory, "Create EffectObject"));
									 },
									 []() {
										 auto obj = SceneAPI::Instantiate<CalyxEngine::FxObject>("EffectObject");
										 obj->Initialize();
										 obj->SetTransient(true);
										 return obj;
									 }});
		}

		// ------------------------------ Event -----------------------------------
		{
			auto& eventItems = categoryItems_[PlaceItemCategory::Event];
			for(const SceneObjectClassDesc* desc : SceneObjectRegistry::Get().ListPlaceableTypes()) {
				if(!desc || desc->objectType != ObjectType::Event) {
					continue;
				}

				const std::string typeName	 = desc->typeName;
				const std::string displayName = desc->displayName.empty() ? desc->typeName : desc->displayName;
				const std::string iconPath	 = desc->iconPath;
				const ObjectType  objectType	 = desc->objectType;

				eventItems.push_back({PlaceItemCategory::Event,
									  displayName,
									  LoadPlaceIcon(iconPath),
									  {64, 64},
									  [typeName, displayName, objectType](const CalyxEngine::Vector3& pos) {
										  auto factory = [typeName, displayName, objectType, pos]() {
											  return CreateRegisteredObjectForScene(
												  typeName, displayName, objectType, pos, false);
										  };
										  CommandManager::GetInstance()->Execute(
											  std::make_unique<CreateObjectCommand<SceneObject>>(
												  SceneContext::Current(), factory, "Create Registered Object"));
									  },
									  [typeName, displayName, objectType]() {
										  return CreateRegisteredObjectForScene(
											  typeName, displayName, objectType, CalyxEngine::Vector3::Zero(), true);
									  }});
			}
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
			float itemWidth	 = 80.0f; // アイコンサイズ(64) + パディング
			int	  columns	 = static_cast<int>(panelWidth / itemWidth);
			if(columns < 1) columns = 1;

			if(ImGui::BeginTable("Grid", columns)) {

				auto drawItem = [](const PlaceItem& item) {
					ImGui::PushID(&item);

					// グループ化して全体をクリッカブルっぽく見せる
					ImGui::BeginGroup();

					// アイコン（画像がなければボタン）
					bool  clicked  = false;
					float iconSize = 64.0f;

					// 中央寄せのためのカーソル操作
					float availW = ImGui::GetContentRegionAvail().x;
					float offX	 = (availW - iconSize) * 0.5f;
					if(offX > 0.0f) ImGui::SetCursorPosX(ImGui::GetCursorPosX() + offX);

					if(item.texture.ptr) {
						clicked = ImGui::ImageButton(
							(ImTextureID)item.texture.ptr,
							ImVec2(iconSize, iconSize));
					} else {
						clicked = ImGui::Button(item.name.c_str(), ImVec2(iconSize, iconSize));
					}

					// テキストも中央寄せ
					{
						float textW	   = ImGui::CalcTextSize(item.name.c_str()).x;
						float textOffX = (availW - textW) * 0.5f;
						if(textOffX > 0.0f) ImGui::SetCursorPosX(ImGui::GetCursorPosX() + textOffX);
						ImGui::TextWrapped("%s", item.name.c_str());
					}

					ImGui::EndGroup();

					// --- Drag and Drop Source ---
					if(ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID)) {
						const PlaceItem* itemPtr = &item;
						ImGui::SetDragDropPayload("DND_PLACE_ITEM", &itemPtr, sizeof(const PlaceItem*));

						// ドラッグ中のプレビュー
						if(item.texture.ptr) {
							ImGui::Image((ImTextureID)item.texture.ptr, ImVec2(32, 32));
						}
						ImGui::SameLine();
						ImGui::Text("%s", item.name.c_str());

						ImGui::EndDragDropSource();
					}

					if(clicked) {
						item.createFunc(CalyxEngine::Vector3::Zero());
					}

					ImGui::PopID();
				};

				// フィルタが有効な場合は全カテゴリから検索
				if(filter_.IsActive()) {
					for(const auto& [cat, items] : categoryItems_) {
						for(const auto& item : items) {
							if(filter_.PassFilter(item.name.c_str())) {
								ImGui::TableNextColumn();
								drawItem(item);
							}
						}
					}
				} else {
					// 選択中のカテゴリのみ表示
					if(categoryItems_.count(selectedCategory_)) {
						const auto& items = categoryItems_[selectedCategory_];
						for(const auto& item : items) {
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
} // namespace CalyxEngine
