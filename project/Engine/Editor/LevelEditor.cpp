#include "LevelEditor.h"

// engine
#include <Engine/Application/Effects/FxSystem.h>
#include <Engine/Application/Effects/Particle/Object/ParticleSystemObject.h>
#include <Engine/Application/System/PlaySession.h>
#include <Engine/Application/UI/EngineUI/Context/EditorContext.h>
#include <Engine/Assets/Database/AssetDatabase.h>
#include <Engine/Editor/PickingPass.h>
#include <Engine/Editor/SceneSwitchOverlay.h>
#include <Engine/Foundation/Input/Input.h>
#include <Engine/Graphics/Camera/Manager/CameraManager.h>
#include <Engine/Objects/3D/Actor/Library/SceneObjectLibrary.h>
#include <Engine/Objects/3D/Actor/SceneObject.h>
#include <Engine/Physics/Ray/Raycastor.h>
#include <Engine/Scene/Context/SceneContext.h>
#include <Engine/Scene/Serializer/SceneSerializer.h>
#include <Engine/Scene/System/SceneManager.h>

// imgui
#include <externals/imgui/ImGuiFileDialog.h>
#include <externals/imgui/imgui.h>

// c++
#include "Engine/Foundation/HotReload/LivePP/LivePPService.h"

#include <Engine/Foundation/Utility/FileSystem/FileScanner.h>
#include <algorithm>

using namespace EngineEdit;

namespace {

	// ライブラリに同一 GUID のオブジェクトが既に登録されているか？
	// （あまり呼ばない前提なので Contains を使う）
	bool LibraryContains(const SceneObjectLibrary*			 lib,
						 const std::shared_ptr<SceneObject>& sp) {
		if(!lib || !sp) return false;
		return lib->Contains(sp->GetGuid());
	}

} // namespace

namespace CalyxEditor {

	//=============================================================================
	// Initialize
	//=============================================================================
	LevelEditor::~LevelEditor() = default;

	void LevelEditor::Initialize() {
#if defined(_DEBUG) || defined(DEVELOP)
		// 各パネルの初期化 ----------------------------------------------------
		hierarchy_			= std::make_unique<HierarchyPanel>();
		editor_				= std::make_unique<EditorPanel>();
		inspector_			= std::make_unique<InspectorPanel>();
		sceneEditor_		= std::make_unique<SceneObjectEditor>();
		placeToolPanel_		= std::make_unique<PlaceToolPanel>();
		splineEditor_		= std::make_unique<SplineEditorPanel>();
		assetPanel_			= std::make_unique<AssetPanel>();
		livePPPanel_		= std::make_unique<LivePPPanel>();
		sceneSwitchOverlay_ = std::make_unique<SceneSwitchOverlay>();

		// レイアウトスイッチャーの初期化 --------------------------------------
		std::string				 layoutDir = "Resources/Assets/Configs/Editor/Layout/";
		auto					 files	   = CalyxUtil::FileScanner::ScanFiles(layoutDir, ".ini");
		std::vector<LayoutEntry> layouts;

		for(const auto& file : files) {
			layouts.push_back({CalyxUtil::FileScanner::GetFileName(file), file.generic_string()});
		}
		// ファイルが見つからなかった場合のフォールバック（念のため）
		if(layouts.empty()) {
			layouts.push_back({"Default", "Resources/Assets/Configs/Editor/Layout/default.ini"});
		}

		layoutSwitcher_ = std::make_unique<ImGuiLayoutSwitcher>(std::move(layouts), "imgui.ini");

		if(auto* db = AssetDatabase::GetInstance()) {
			assetPanel_->Initialize(db->GetRoot());
		}

		// Panel に LevelEditor 自体を渡す（コールバック通知や setter） ----------
		editor_->SetOnEditorSelected(
			[this](BaseEditor* ed) { SetSelectedEditor(ed); });

		// Hierarchy から来るコールバックは shared_ptr で受けて、
		// LevelEditor 内で weak_ptr に変換して管理する
		hierarchy_->SetOnObjectSelected(
			[this](std::shared_ptr<SceneObject> sp) { SetSelectedObject(sp); });

		hierarchy_->SetOnObjectDelete(
			[this](std::shared_ptr<SceneObject> sp) { DeleteObject(std::move(sp)); });

		hierarchy_->SetOnObjectCreate(
			[this](std::shared_ptr<SceneObject> sp) { CreateObject(std::move(sp)); });

		inspector_->SetSceneObjectEditor(sceneEditor_.get());

		// ビューポートの初期化 ------------------------------------------------
		mainViewport_	 = std::make_unique<Viewport>(ViewportType::VIEWPORT_MAIN, "Game Viewport");
		debugViewport_	 = std::make_unique<Viewport>(ViewportType::VIEWPORT_DEBUG, "Debug Viewport");
		pickingViewport_ = std::make_unique<Viewport>(ViewportType::VIEWPORT_PICKING, "Picking Viewport");

		performanceOverlay_ = std::make_unique<PerformanceOverlay>();
		debugOverlay_		= std::make_unique<DebugOverlay>();

		// Manipulator をツールとして登録
		if(auto* manipulator = sceneEditor_->GetManipulator()) {
			debugViewport_->AddTool(manipulator);
			debugViewport_->AddTool(performanceOverlay_.get());
			debugViewport_->AddTool(debugOverlay_.get());
		}

		// エディターメニューの初期化 ------------------------------------------
		menu_ = std::make_unique<EditorMenu>();

		// --- Advanced Hot Reload (Object Re-instancing) ---
		if(auto* lpp = CalyxEngine::LivePPService::GetInstance()) {
			lpp->AddPrePatchListener([this]() {
				if(auto* ctx = SceneContext::Current()) {
					livePPSnapshot_ = SceneSerializer::DumpJson(*ctx);
					OutputDebugStringW(L"[LivePP] Scene snapshot taken.\n");
				}
			});

			lpp->AddPostPatchListener([this]() {
				if(auto* ctx = SceneContext::Current()) {
					if(!livePPSnapshot_.empty()) {
						ClearSelection();
						ctx->Clear();
						SceneSerializer::LoadJson(*ctx, livePPSnapshot_);
						livePPSnapshot_.clear();
						OutputDebugStringW(L"[LivePP] Scene re-instanced from snapshot.\n");
					}
				}
			});
		}

		// File: Save Scene
		menu_->Add(MenuCategory::File,
				   {"Save Scene",
					"Ctrl+S",
					[this]() {
						IGFD::FileDialogConfig config;
						config.path = "Resources/Assets/Scenes/";
						ImGuiFileDialog::Instance()->OpenDialog(
							"SceneSaveDialog",
							"save scene file",
							".scene",
							config);
					},
					true});

		// File: Open Scene
		menu_->Add(MenuCategory::File,
				   {"Open Scene",
					"Ctrl+O",
					[] {
						IGFD::FileDialogConfig config;
						config.path = "Resources/Assets/Scenes/";
						ImGuiFileDialog::Instance()->OpenDialog(
							"SceneOpenDialog",
							"open scene",
							".scene",
							config);
					},
					true});

		// View: Game Mode トグル
		if(mode_ == EditorMode::Edit) {
			menu_->Add(MenuCategory::View,
					   {"Enter Game Mode", "", [this]() { ToggleMode(); }, true});
		} else {
			menu_->Add(MenuCategory::View,
					   {"Exit Game Mode", "", [this]() { ToggleMode(); }, true});
		}

		// パネル群を登録（Editors メニューに並べる） --------------------------
		editorPanels_.push_back(hierarchy_.get());
		editorPanels_.push_back(editor_.get());
		editorPanels_.push_back(inspector_.get());
		editorPanels_.push_back(placeToolPanel_.get());
		editorPanels_.push_back(splineEditor_.get());
		editorPanels_.push_back(assetPanel_.get());
		editorPanels_.push_back(livePPPanel_.get());

		// Editors メニュー（MenuCategory::Tools）に各パネルのトグルを追加
		for(auto* p : editorPanels_) {
			// LivePPPanel は自動表示なのでメニューには出さない
			if(p == livePPPanel_.get()) continue;

			menu_->Add(MenuCategory::Tools,
					   {p->GetPanelName(),
						"",
						[p, this]() { TogglePanel(p); },
						true});
		}

		// Viewport 表示トグル
		menu_->Add(MenuCategory::View,
				   {"Main Viewport",
					"",
					[this] {
						if(mainViewport_) {
							mainViewport_->SetShow(!mainViewport_->IsShow());
						}
					},
					true});

		menu_->Add(MenuCategory::View,
				   {"Debug Viewport",
					"",
					[this] {
						if(debugViewport_) {
							debugViewport_->SetShow(!debugViewport_->IsShow());
						}
					},
					true});

		// Play / Pause / Exit（PlaySession 連携） ------------------------------
		menu_->Add(MenuCategory::Edit,
				   {"Play ",
					"(F5)",
					[this] {
						if(pPlaySesseion_ && !pPlaySesseion_->IsRuntime()) {
							pPlaySesseion_->Enter();
						}
					},
					true});

		menu_->Add(MenuCategory::Edit,
				   {"Pause ",
					"(F6)",
					[this] {
						if(pPlaySesseion_ && pPlaySesseion_->IsRuntime()) {
							pPlaySesseion_->TogglePause();
						}
					},
					true});

		menu_->Add(MenuCategory::Edit,
				   {"Exit ",
					"(Shift+F5)",
					[this] {
						if(pPlaySesseion_ && pPlaySesseion_->IsRuntime()) {
							pPlaySesseion_->Exit();
						}
					},
					true});

		// Edit: Hot Reload
		menu_->Add(MenuCategory::Edit,
				   {"Hot Reload (Live++)",
					"Ctrl+Alt+F11",
					[] {
						if(auto* service = CalyxEngine::LivePPService::GetInstance()) {
							service->TriggerReload();
						}
					},
					true});

		// Scene: Switch Scene
#endif // _DEBUG || DEVELOP
	}

	//=============================================================================
	// Update
	//=============================================================================
	void LevelEditor::Update() {
#if defined(_DEBUG) || defined(DEVELOP)
		SceneContext* ctx = SceneContext::Current();

		const ImGuiIO& io			= ImGui::GetIO();
		const bool	   guizmoActive = ImGuizmo::IsOver() || ImGuizmo::IsUsing();

		// --- デバッグビューポート上にマウスがあるか？ -----------------------
		bool overDebugViewport = false;
		if(debugViewport_ && debugViewport_->IsShow()) {
			overDebugViewport = debugViewport_->IsHovered();
		}

		// ImGui がマウスを掴んでいても、ビューポート上なら許可
		const bool uiBlocksClick = io.WantCaptureMouse && !overDebugViewport;

		if(auto* debugCam = CameraManager::GetDebug()) {
			debugCam->SetInputEnabled(overDebugViewport);
		}

		if(debugViewport_ && debugViewport_->IsShow() && !guizmoActive && !uiBlocksClick) {
			const bool imguiEdge = ImGui::IsMouseClicked(ImGuiMouseButton_Left);

			static bool prevDILeft = false;
			const bool	diNow	   = CalyxFoundation::Input::GetInstance()->PushMouseButton(CalyxFoundation::MouseButton::Left);
			const bool	diEdge	   = diNow && !prevDILeft;
			prevDILeft			   = diNow;

			if(imguiEdge || diEdge) {
				TryPickUnderCursor();
			}
		}

		// ----------------------------
		// Open Scene ダイアログ処理
		// ----------------------------
		if(ImGuiFileDialog::Instance()->Display("SceneOpenDialog")) {
			if(ImGuiFileDialog::Instance()->IsOk()) {
				std::string filePath = ImGuiFileDialog::Instance()->GetFilePathName();
				ClearSelection();
				SceneSerializer::Load(*ctx, filePath);
			}
			ImGuiFileDialog::Instance()->Close();
		}

		// ----------------------------
		// Save Scene ダイアログ処理
		// ----------------------------
		if(ImGuiFileDialog::Instance()->Display("SceneSaveDialog")) {
			if(ImGuiFileDialog::Instance()->IsOk()) {
				std::string filePath = ImGuiFileDialog::Instance()->GetFilePathName();
				SceneSerializer::Save(*ctx, filePath);
			}
			ImGuiFileDialog::Instance()->Close();
		}

		// シーンコンテキスト切り替え検出
		NotifySceneContextChanged();
		prevCtx_ = SceneContext::Current();

		// PlaySession 状態と EditorMode の同期 ------------------------------
		if(pPlaySesseion_) {
			const bool playing = pPlaySesseion_->IsRuntime();
			if(playing && !lastPlaying_) {
				EnterGameMode();
			} else if(!playing && lastPlaying_) {
				ExitGameMode();
			}
			lastPlaying_ = playing;
		}

		// LivePP Visibility Control
		if(livePPPanel_) {
			auto* service	 = CalyxEngine::LivePPService::GetInstance();
			bool  shouldShow = (service && service->GetStatus() != CalyxEngine::LivePPStatus::Idle);
			livePPPanel_->SetShow(shouldShow);
		}
#endif
	}

	//=============================================================================
	// Render
	//=============================================================================
	void LevelEditor::Render() {
		// 各パネル描画
		for(auto* p : editorPanels_) {
			if(p->IsShow()) {
				p->Render();
			}
		}

		// Play セッション用ツールバー
		if(pPlaySesseion_) {
			pPlaySesseion_->RenderToolbar();
		}

		// シーン切り替えツールバー
		if(sceneSwitchOverlay_) {
			sceneSwitchOverlay_->RenderToolbar();
		}

		// パフォーマンス表示
		if(performanceOverlay_) {
			performanceOverlay_->RenderToolbar();
		}

		// SceneObjectEditor 側の更新（マニピュレータなど）
		if(sceneEditor_) {
			sceneEditor_->Update();
		}
	}

	//=============================================================================
	// Menu
	//=============================================================================
	void LevelEditor::RenderMenu() {
		if(menu_) {
			menu_->Render();
		}
		// レイアウトスイッチャーメニューの描画
		if(layoutSwitcher_ && ImGui::BeginMainMenuBar()) {
			layoutSwitcher_->DrawMenu();
			ImGui::EndMainMenuBar();
		}
	}

	//=============================================================================
	// Game Mode 切り替え
	//=============================================================================
	void LevelEditor::EnterGameMode() {
		mode_ = EditorMode::Game;
	}

	void LevelEditor::ExitGameMode() {
		mode_ = EditorMode::Edit;

		if(pPlaySesseion_ && pPlaySesseion_->IsRuntime()) {
			pPlaySesseion_->Exit();
			lastPlaying_ = false;
		}
	}

	void LevelEditor::ToggleMode() {
		if(mode_ == EditorMode::Edit) {
			mode_ = EditorMode::Game;
		} else {
			mode_ = EditorMode::Edit;
		}
	}

	//=============================================================================
	// Selection API
	//=============================================================================
	void LevelEditor::SetSelectedEditor(BaseEditor* editor) {
		selectedEditor_ = editor;
		selectedObject_.reset();

		if(inspector_) {
			inspector_->SetSelectedEditor(editor);
			// オブジェクト選択はクリア
			inspector_->SetSelectedObject(std::shared_ptr<SceneObject>{});
		}
		if(sceneEditor_) {
			sceneEditor_->ClearSelection();
		}
	}

	void LevelEditor::SetSelectedObject(const std::shared_ptr<SceneObject>& sp) {
		// 内部では weak_ptr として保持
		selectedObject_ = sp;
		selectedEditor_ = nullptr;

		// Hierarchy / Inspector / SceneObjectEditor にも通知
		if(hierarchy_) {
			hierarchy_->SetSelectedObject(sp);
		}
		if(inspector_) {
			inspector_->SetSelectedEditor(nullptr);
			inspector_->SetSelectedObject(sp);
		}
		if(sceneEditor_) {
			sceneEditor_->SetTarget(sp ? sp.get() : nullptr);
		}

		if(auto* ctx = SceneContext::Current()) {
			ctx->SetDebugSelectedObject(sp ? sp.get() : nullptr);
		}
	}

	//=============================================================================
	// Create / Delete Object
	//=============================================================================
	void LevelEditor::CreateObject(const std::shared_ptr<SceneObject>& obj) {
		if(!obj) return;

		SceneContext* ctx = SceneContext::Current();
		if(!ctx) return;

		auto* lib = ctx->GetObjectLibrary();
		if(!lib) return;

		// すでに同一 GUID が登録済みなら何もしない（Prefab の重複登録防止）
		if(LibraryContains(lib, obj)) {
			return;
		}

		// SceneContext 経由で登録（内部で SceneObjectLibrary::AddObject を呼ぶ）
		ctx->AddObject(obj);

		// パーティクルなら FxSystem 側にも登録
		if(obj->GetObjectType() == ObjectType::Effect) {
			if(auto fxObj = std::dynamic_pointer_cast<CalyxEffect::ParticleSystemObject>(obj)) {
				if(auto* fxSys = ctx->GetFxSystem()) {
					fxSys->AddEmitter(fxObj->GetEmitter(), fxObj->GetGuid());
				}
			}
		}

		if(hierarchy_) hierarchy_->RefreshCache();
	}

	void LevelEditor::DeleteObject(const std::shared_ptr<SceneObject>& sp) {
		if(!sp) return;

		SceneContext* ctx = SceneContext::Current();
		if(!ctx) return;

		// ── 選択状態をクリア（選択中だった場合） ─────────────────────
		if(auto sel = selectedObject_.lock()) {
			if(sel == sp) {
				ClearSelection();
			}
		}

		// ── パーティクルシステムなら FxSystem からも削除 ─────────────
		if(sp->GetObjectType() == ObjectType::Effect) {
			if(auto fxObj = std::dynamic_pointer_cast<CalyxEffect::ParticleSystemObject>(sp)) {
				if(auto* fxSys = ctx->GetFxSystem()) {
					fxSys->RemoveEmitter(fxObj->GetEmitter().get());
				}
			}
		}

		// ── SceneContext 経由で削除 ─────────────────────────────────
		// 内部で SceneObjectLibrary::RemoveObject が呼ばれ、
		// さらに SceneContext::objectRemovedCallbacks_ も通知される。
		ctx->RemoveObject(sp);

		if(hierarchy_) hierarchy_->RefreshCache();
	}

	//=============================================================================
	// Viewport
	//=============================================================================
	void LevelEditor::RenderViewport(ViewportType type, const ImTextureID& tex) {
		if(type == ViewportType::VIEWPORT_MAIN) {
			if(mainViewport_ && mainViewport_->IsShow()) {
				mainViewport_->Render(tex);
			}
		} else if(type == ViewportType::VIEWPORT_DEBUG) {
			if(debugViewport_ && debugViewport_->IsShow()) {
				debugViewport_->Render(tex);
			}
		} else if(type == ViewportType::VIEWPORT_PICKING) {
			if(pickingViewport_ && pickingViewport_->IsShow()) {
				pickingViewport_->Render(tex);
			}
		}
	}

	void LevelEditor::SetCameraForViewport(BaseCamera* mainCamera, BaseCamera* debugCamera) {
		if(mainViewport_) {
			mainViewport_->SetCamera(mainCamera);
		}
		if(debugViewport_) {
			debugViewport_->SetCamera(debugCamera);
		}
	}

	//=============================================================================
	// Picking
	//=============================================================================
	void LevelEditor::TryPickObjectFromMouse(const CalyxMath::Vector2&	 mouse,
											 const CalyxMath::Vector2&	 viewportSize,
											 const CalyxMath::Matrix4x4& view,
											 const CalyxMath::Matrix4x4& proj) {
		SceneContext* ctx = SceneContext::Current();
		if(!ctx || !debugViewport_) return;

		// ビューポート内ローカル座標へ変換
		CalyxMath::Vector2 mouseLocal = mouse - debugViewport_->GetPosition();

		Ray ray = Raycastor::ConvertMouseToRay(mouseLocal, view, proj, viewportSize);

		// ヒット判定（raw ptr）
		if(SceneObject* raw = PickSceneObjectByRay(ray)) {
			// 対応する shared_ptr をライブラリから取得
			if(auto sp = ctx->FindSharedObject(raw)) {
				SetSelectedObject(sp);
			}
		}
	}

	SceneObject* LevelEditor::PickSceneObjectByRay(const Ray& ray) {
		const auto* lib = hierarchy_ ? hierarchy_->GetSceneObjectLibrary() : nullptr;
		if(!lib) return nullptr;

		const auto& allObjects = lib->GetAllObjectsRaw();
		auto		hit		   = Raycastor::Raycast(ray, allObjects);
		if(hit) {
			return static_cast<SceneObject*>(hit->hitObject);
		}
		return nullptr;
	}

	void LevelEditor::TryPickUnderCursor() {
		if(!debugViewport_ || !debugViewport_->IsShow()) return;

		SceneContext* current = SceneContext::Current();
		if(!current) return;

		CalyxMath::Vector2 origin = debugViewport_->GetPosition();
		CalyxMath::Vector2 size	  = debugViewport_->GetSize();

		ImVec2 mouse	 = ImGui::GetMousePos();
		float  relativeX = mouse.x - origin.x;
		float  relativeY = mouse.y - origin.y;

		if(relativeX < 0 || relativeY < 0 || relativeX > size.x || relativeY > size.y) return;

		// --- Pixel Shader Picking (Priority) ---
		if(sceneManager_) {
			if(auto* pickingPass = sceneManager_->GetPickingPass()) {
				// ビューポートサイズとテクスチャサイズの比率を計算して座標をスケーリング
				float scaleX = static_cast<float>(pickingPass->GetWidth()) / size.x;
				float scaleY = static_cast<float>(pickingPass->GetHeight()) / size.y;

				int32_t px = static_cast<int32_t>(relativeX * scaleX);
				int32_t py = static_cast<int32_t>(relativeY * scaleY);

				uint32_t pickingID = pickingPass->GetObjectID(px, py);
				if(pickingID > 0) {
					if(auto sp = current->GetObjectLibrary()->FindSharedByPickingID(pickingID)) {
						// 地面だったらリターン
						if(sp->GetName() == "ground") {
							return;
						}
						SetSelectedObject(sp);
						return;
					}
				}
			}
		}

		// --- Raycast Picking (Fallback) ---
		CalyxMath::Vector2	 mousePos(relativeX, relativeY);
		CalyxMath::Matrix4x4 view = CameraManager::GetDebug()->GetViewMatrix();
		CalyxMath::Matrix4x4 proj = CameraManager::GetDebug()->GetProjectionMatrix();

		Ray ray = Raycastor::ConvertMouseToRay(mousePos, view, proj, size);
		if(SceneObject* picked = PickSceneObjectByRay(ray)) {
			if(auto sp = current->FindSharedObject(picked)) {
				SetSelectedObject(sp);
			}
		}
	}

	//=============================================================================
	// Scene Save
	//=============================================================================
	void LevelEditor::SaveScene() {
		SceneContext* ctx = SceneContext::Current();
		if(!ctx) return;

		std::string scenePath = "Resources/Assets/Scenes/" + ctx->GetSceneName() + ".scene";
		SceneSerializer::Save(*ctx, scenePath);
	}

	//=============================================================================
	// SceneContext の変更検出
	//=============================================================================
	void LevelEditor::SetSceneManager(CalyxScene::SceneManager* manager) {
		sceneManager_ = manager;

		if(manager) {
			auto* pickingPass = manager->GetPickingPass();
			if(mainViewport_) mainViewport_->SetPickingPass(pickingPass);
			if(debugViewport_) debugViewport_->SetPickingPass(pickingPass);
			if(pickingViewport_) pickingViewport_->SetPickingPass(pickingPass);
		}

		if(sceneSwitchOverlay_) {
			sceneSwitchOverlay_->SetSceneManager(manager);
		}
	}
	void LevelEditor::NotifySceneContextChanged() {
		SceneContext* current = SceneContext::Current();
		if(prevCtx_ == current) return;

		if(hierarchy_) {
			hierarchy_->SetSceneObjectLibrary(current ? current->GetObjectLibrary() : nullptr);
			hierarchy_->RefreshCache();
		}

		ClearSelection();

		if(current) {
			current->AddOnObjectRemovedListener(
				[editor = this](SceneObject* removed) {
					if(!editor) return;

					auto sel = editor->GetHierarchyPanel()->GetSelectedObject(); // weak_ptr
					if(sel.lock().get() == removed) {
						editor->SetSelectedObject(std::shared_ptr<SceneObject>{});
					}
				});

			if(sceneEditor_) {
				sceneEditor_->BindRemovalCallback(current);
			}
		}
	}
	void LevelEditor::ClearSelection() {
		selectedObject_.reset();
		selectedEditor_ = nullptr;

		std::weak_ptr<SceneObject> empty;

		hierarchy_->SetSelectedObject(empty);
		inspector_->SetSelectedObject(empty);
		sceneEditor_->ClearSelection();

		if(auto* ctx = SceneContext::Current()) {
			ctx->SetDebugSelectedObject(nullptr);
		}
	}

} // namespace CalyxEditor