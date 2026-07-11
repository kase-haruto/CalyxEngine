#include "LevelEditor.h"

// engine
#include <CalyxEngine/Project.h>
#include <externals/nlohmann/json.hpp>
#include <Engine/Application/Effects/FxObject.h>
#include <Engine/Application/Settings/EngineSettings.h>
#include <Engine/Foundation/Clock/ClockManager.h>
#include <Engine/Application/System/PlaySession.h>
#include <Engine/Application/UI/EngineUI/Context/EditorContext.h>
#include <Engine/Assets/Database/AssetDatabase.h>
#include <Engine/Editor/ParticlePreviewSession.h>
#include <Engine/Editor/PrefabEditSession.h>
#include <Engine/Editor/SceneObjectEditCommands.h>
#include <Engine/Editor/SceneObjectDuplicator.h>
#include <Engine/Editor/SceneSwitchOverlay.h>
#include <Engine/Editor/ViewportSelectionController.h>
#include <Engine/Foundation/Input/Input.h>
#include <Engine/Foundation/Log/EngineLogger.h>
#include <Engine/Graphics/Camera/Manager/CameraManager.h>
#include <Engine/Objects/3D/Actor/Library/SceneObjectLibrary.h>
#include <Engine/Objects/3D/Actor/SceneObject.h>
#include <Engine/Scene/Context/SceneContext.h>
#include <Engine/Scene/Serializer/SceneSerializer.h>
#include <Engine/Scene/Settings/SceneSettingsWindow.h>
#include <Engine/Scene/System/SceneManager.h>
#include <Engine/System/Command/Manager/CommandManager.h>

// imgui
#include <externals/imgui/ImGuiFileDialog.h>
#include <externals/imgui/imgui.h>

// c++
#include "Engine/Foundation/HotReload/LivePP/LivePPService.h"

#include <Engine/Foundation/Utility/FileSystem/FileScanner.h>
#include <algorithm>
#include <cctype>
#include <filesystem>
#include <functional>
#include <system_error>
#include <unordered_map>

using namespace EngineEdit;

namespace {

	// ライブラリに同一 GUID のオブジェクトが既に登録されているか？
	// （あまり呼ばない前提なので Contains を使う）
	bool LibraryContains(const SceneObjectLibrary*			 lib,
						 const std::shared_ptr<SceneObject>& sp) {
		if(!lib || !sp) return false;
		return lib->Contains(sp->GetGuid());
	}

	bool IsSceneFilePath(std::filesystem::path path) {
		auto ext = path.extension().string();
		std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char c) {
			return static_cast<char>(std::tolower(c));
		});
		return ext == ".scene";
	}

	std::filesystem::path NormalizeSceneCreatePath(std::filesystem::path requestedDestination) {
		if(!IsSceneFilePath(requestedDestination)) {
			requestedDestination += ".scene";
		}

		std::error_code ec;
		auto parent = requestedDestination.parent_path();
		if(!parent.empty()) {
			parent = std::filesystem::weakly_canonical(parent, ec);
			if(!ec) {
				requestedDestination = parent / requestedDestination.filename();
			}
		}
		return requestedDestination.lexically_normal();
	}

	std::filesystem::path FindDefaultSceneTemplate() {
		const auto templatePath = Calyx::ResolveAssetPath("Scenes/DefaultScene.scene");
		std::error_code ec;
		if(std::filesystem::is_regular_file(templatePath, ec)) {
			return std::filesystem::weakly_canonical(templatePath, ec);
		}
		return {};
	}

	void RegenerateTemplateSceneGuids(SceneContext& context) {
		auto* library = context.GetObjectLibrary();
		if(!library) return;

		std::unordered_map<Guid, Guid> guidMap;
		for(const auto& object : library->GetAllObjectsShared()) {
			if(!object) continue;
			guidMap[object->GetGuid()] = Guid::New();
		}

		// テンプレートから作成したシーンがDefaultSceneと同じGUIDを共有しないようにする。
		for(const auto& object : library->GetAllObjectsShared()) {
			if(!object) continue;
			const auto it = guidMap.find(object->GetGuid());
			if(it != guidMap.end()) {
				object->SetGuid(it->second);
			}
		}
	}

} // namespace

namespace CalyxEngine {

	//=============================================================================
	// Initialize
	//=============================================================================
	LevelEditor::LevelEditor() = default;
	LevelEditor::~LevelEditor() = default;

	void LevelEditor::Initialize() {
#if defined(_DEBUG) || defined(DEVELOP)
		// 各パネルの初期化 ----------------------------------------------------
		hierarchy_			= std::make_unique<HierarchyPanel>();
		inspector_			= std::make_unique<InspectorPanel>();
		keyframePanel_		= std::make_unique<KeyframePanel>();
		sceneEditor_		= std::make_unique<SceneObjectEditor>();
		placeToolPanel_		= std::make_unique<PlaceToolPanel>();
		splineEditor_		= std::make_unique<SplineEditorPanel>();
		assetPanel_			= std::make_unique<AssetPanel>();
		materialNodeEditorPanel_ = std::make_unique<MaterialNodeEditorPanel>();
		postEffectNodeEditorPanel_ = std::make_unique<PostEffectNodeEditorPanel>();
		spriteAnimationEditorPanel_ = std::make_unique<SpriteAnimationEditorPanel>();
		livePPPanel_		= std::make_unique<LivePPPanel>();
		logPanel_			= std::make_unique<LogPanel>();
		logPanel_->SetCommandContext({this, sceneManager_, pPlaySesseion_});
		sceneSwitchOverlay_ = std::make_unique<SceneSwitchOverlay>();
		sceneSettingsWindow_ = std::make_unique<SceneSettingsWindow>();
		debugCameraFocus_	= std::make_unique<DebugCameraFocusController>();
		particlePreview_ = std::make_unique<ParticlePreviewSession>();
		prefabEdit_		= std::make_unique<PrefabEditSession>();
		viewportSelection_ = std::make_unique<ViewportSelectionController>();

		// レイアウトスイッチャーの初期化 --------------------------------------
		std::string				 layoutDir = Calyx::ResolveAssetPath("Configs/Editor/Layout").generic_string();
		auto					 files	   = CalyxEngine::FileScanner::ScanFiles(layoutDir, ".ini");
		std::vector<LayoutEntry> layouts;

		for(const auto& file : files) {
			layouts.push_back({CalyxEngine::FileScanner::GetFileName(file), file.generic_string()});
		}
		// ファイルが見つからなかった場合のフォールバック（念のため）
		if(layouts.empty()) {
			layouts.push_back({"Default", Calyx::ResolveAssetPath("Configs/Editor/Layout/default.ini").generic_string()});
		}

		layoutSwitcher_ = std::make_unique<ImGuiLayoutSwitcher>(std::move(layouts), "imgui.ini");
		editorToolRegistry_.SetWorkspaceRequest([this](const std::string& layoutPath) {
			if(layoutSwitcher_) layoutSwitcher_->Apply(layoutPath);
		});

		if(auto* db = AssetDatabase::GetInstance()) {
			assetPanel_->Initialize(db->GetRoot());
		}
		assetPanel_->SetOnPrefabEditRequested(
			[this](const std::filesystem::path& path) {
				OpenPrefabForEdit(path.string());
			});
		assetPanel_->SetOnSceneOpenRequested(
			[this](const std::filesystem::path& path) {
				OpenScene(path);
			});

		// Hierarchy から来るコールバックは shared_ptr で受けて、
		// LevelEditor 内で weak_ptr に変換して管理する
		hierarchy_->SetOnObjectSelected(
			[this](std::shared_ptr<SceneObject> sp, bool toggle) {
				if(toggle) {
					ToggleSelectedObject(sp);
				} else {
					SetSelectedObject(sp);
				}
			});

		hierarchy_->SetOnObjectDelete(
			[this](std::shared_ptr<SceneObject> sp) { DeleteObject(std::move(sp)); });

		hierarchy_->SetOnObjectCreate(
			[this](std::shared_ptr<SceneObject> sp) { CreateObject(std::move(sp)); });

		hierarchy_->SetOnObjectRename(
			[](std::shared_ptr<SceneObject> sp, const std::string& newName) {
				if(!sp) return;
				if(auto* ctx = SceneContext::Current()) {
					if(auto* lib = ctx->GetObjectLibrary()) {
						lib->RenameObject(sp, newName);
						return;
					}
				}
				sp->SetName(newName, sp->GetObjectType());
			});
		hierarchy_->SetOnApplyPrefabOverrides(
			[this](std::shared_ptr<SceneObject> sp) {
				ApplyPrefabOverridesFromInstance(sp);
			});
		hierarchy_->SetOnObjectFocused(
			[this](std::shared_ptr<SceneObject> sp) {
				if(debugCameraFocus_) {
					debugCameraFocus_->StartFocus(CameraManager::GetDebug(), sp);
				}
			});

		inspector_->SetSceneObjectEditor(sceneEditor_.get());
		keyframePanel_->SetSelectionProvider([this]() {
			return GetSelectedObjects();
		});
		selection_.Bind(hierarchy_.get(), inspector_.get(), sceneEditor_.get());

		// ビューポートの初期化 ------------------------------------------------
		mainViewport_	 = std::make_unique<Viewport>(ViewportType::VIEWPORT_MAIN, "Game Viewport");
		debugViewport_	 = std::make_unique<Viewport>(ViewportType::VIEWPORT_DEBUG, "Debug Viewport");
		pickingViewport_ = std::make_unique<Viewport>(ViewportType::VIEWPORT_PICKING, "Picking Viewport");
		pickingDepthViewport_ = std::make_unique<Viewport>(ViewportType::VIEWPORT_PICKING_DEPTH, "Picking Depth Viewport");
		pickingViewport_->SetShow(false);
		pickingDepthViewport_->SetShow(false);
		viewportSelection_->SetViewport(debugViewport_.get());
		viewportSelection_->SetCallbacks(ViewportSelectionCallbacks{
			[this]() { return GetSelectedObjects(); },
			[this](const std::shared_ptr<SceneObject>& object) { SetSelectedObject(object); },
			[this](const std::shared_ptr<SceneObject>& object) { ToggleSelectedObject(object); },
			[this](const std::vector<std::shared_ptr<SceneObject>>& objects) { SetSelectedObjects(objects); }});

		performanceOverlay_ = std::make_unique<PerformanceOverlay>();
		debugOverlay_		= std::make_unique<DebugOverlay>();

		// Manipulator をツールとして登録
		if(auto* manipulator = sceneEditor_->GetManipulator()) {
			manipulator->SetOnCtrlTranslateDuplicate([this]() {
				return DuplicateSelectedObjects();
			});
			mainViewport_->AddTool(manipulator);
			debugViewport_->AddTool(manipulator);
			debugViewport_->AddTool(performanceOverlay_.get());
			debugViewport_->AddTool(debugOverlay_.get());
		}
		if(splineEditor_) {
			debugViewport_->AddTool(splineEditor_.get());
		}

		// エディターメニューの初期化 ------------------------------------------
		menu_ = std::make_unique<EditorMenu>();
		menu_->SetToolExtensionRenderer([this]() { editorToolRegistry_.DrawToolsMenu(); });
		EngineSettings::GetInstance()->Initialize();
		if(auto* manipulator = sceneEditor_->GetManipulator()) {
			manipulator->ApplySettings(EngineSettings::GetInstance()->GetData().manipulator);
		}

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

		menu_->Add(MenuCategory::File,
				   {"New Scene",
					"Ctrl+N",
					[] {
						IGFD::FileDialogConfig config;
						config.path = Calyx::ResolveAssetPath("Scenes").generic_string();
						config.fileName = "NewScene.scene";
						ImGuiFileDialog::Instance()->OpenDialog(
							"SceneCreateDialog",
							"new scene",
							".scene",
							config);
					},
					true});

		// File: Save Scene
		menu_->Add(MenuCategory::File,
				   {"Save Scene",
					"Ctrl+S",
					[this]() {
						IGFD::FileDialogConfig config;
						config.path = Calyx::ResolveAssetPath("Scenes").generic_string();
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
						config.path = Calyx::ResolveAssetPath("Scenes").generic_string();
						ImGuiFileDialog::Instance()->OpenDialog(
							"SceneOpenDialog",
							"open scene",
							".scene",
							config);
					},
					true});

		menu_->Add(MenuCategory::File,
				   {"New Prefab",
					"",
					[this] {
						ApplyEditToolMode(EngineEdit::EditToolMode::Prefab, true);
						NewPrefabEditContext();
					},
					true});

		menu_->Add(MenuCategory::File,
				   {"Open Prefab",
					"",
					[] {
						IGFD::FileDialogConfig config;
						config.path = Calyx::GetAssetRoot().generic_string();
						ImGuiFileDialog::Instance()->OpenDialog(
							"PrefabOpenDialog",
							"open prefab",
							".prefab",
							config);
					},
					true});

		menu_->Add(MenuCategory::File,
				   {"Save Prefab",
					"",
					[this] { SavePrefabEdit(); },
					true});

		menu_->Add(MenuCategory::File,
				   {"Save Prefab As",
					"",
					[] {
						IGFD::FileDialogConfig config;
						config.path = Calyx::ResolveAssetPath("Prefabs").generic_string();
						ImGuiFileDialog::Instance()->OpenDialog(
							"PrefabSaveAsDialog",
							"save prefab file",
							".prefab",
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
		editorPanels_.push_back(inspector_.get());
		editorPanels_.push_back(keyframePanel_.get());
		editorPanels_.push_back(placeToolPanel_.get());
		editorPanels_.push_back(splineEditor_.get());
		editorPanels_.push_back(assetPanel_.get());
		editorPanels_.push_back(materialNodeEditorPanel_.get());
		editorPanels_.push_back(postEffectNodeEditorPanel_.get());
		editorPanels_.push_back(spriteAnimationEditorPanel_.get());
		editorPanels_.push_back(logPanel_.get());
		editorPanels_.push_back(livePPPanel_.get());

		ApplyEditToolMode(editToolMode_, true);

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

		// ログパネルまで登録されたことを、動作確認用の初期ログとして記録する。
		EngineLogger::GetInstance().Add(
			LogLevel::Info,
			LogCategory::Editor,
			"LogPanel initialized.",
			"LevelEditor");

		menu_->Add(MenuCategory::Settings,
				   {"Engine Settings",
					"",
					[] { EngineSettings::GetInstance()->OpenSettingsWindow(); },
					true});

		menu_->Add(MenuCategory::Settings,
				   {"Scene Settings",
					"",
					[this] {
						if(sceneSettingsWindow_) {
							sceneSettingsWindow_->Open();
						}
					},
					true});

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

		menu_->Add(MenuCategory::View,
				   {"Picking Viewport",
					"",
					[this] {
						if(pickingViewport_) {
							pickingViewport_->SetShow(!pickingViewport_->IsShow());
						}
					},
					true});

		menu_->Add(MenuCategory::View,
				   {"Picking Depth Viewport",
					"",
					[this] {
						if(pickingDepthViewport_) {
							pickingDepthViewport_->SetShow(!pickingDepthViewport_->IsShow());
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
		if(layoutSwitcher_) {
			layoutSwitcher_->ApplyPending();
		}
		editorToolRegistry_.Update();
		const float dt = ClockManager::GetInstance()->GetDeltaTime();

		auto notifySceneSaved = [this](const std::string& path) {
			sceneSavedPopupPath_ = path;
			sceneSavedPopupTimer_ = 1.5f;
			ImGui::OpenPopup("SceneSavedPopup");
			EngineLogger::GetInstance().Add(
				LogLevel::Info,
				LogCategory::Editor,
				"Scene saved: " + path,
				"LevelEditor");
		};

		if(editToolMode_ == EngineEdit::EditToolMode::ParticleEffect) {
			UpdateParticlePreviewContext(dt);
		} else if(editToolMode_ == EngineEdit::EditToolMode::Prefab) {
			UpdatePrefabEditContext(dt);
		}
		ActivateModeContext(editToolMode_);

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

		if(debugCameraFocus_) {
			debugCameraFocus_->Update(ClockManager::GetInstance()->GetDeltaTime());
		}

		if(auto* debugCam = CameraManager::GetDebug()) {
			const bool focusMoving = debugCameraFocus_ && debugCameraFocus_->IsActive();
			debugCam->SetInputEnabled(overDebugViewport && !focusMoving);
		}

		if(debugViewport_ && debugViewport_->IsShow() && !guizmoActive && !uiBlocksClick) {
			if(viewportSelection_) viewportSelection_->UpdateInput();
		} else if(!ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
			if(viewportSelection_) viewportSelection_->CancelRangeSelection();
		}

		if(selection_.HasSelection() && !io.WantTextInput && ImGui::IsKeyPressed(ImGuiKey_Delete)) {
			DeleteSelectedObjects();
		}

		if(selection_.HasSelection() &&
		   !io.WantTextInput &&
		   io.KeyCtrl &&
		   !io.KeyAlt &&
		   !io.KeyShift &&
		   ImGui::IsKeyPressed(ImGuiKey_D, false)) {
			DuplicateSelectedObjects();
		}

		// ----------------------------
		// Open Scene ダイアログ処理
		// ----------------------------
		if(ImGuiFileDialog::Instance()->Display("SceneOpenDialog")) {
			if(ImGuiFileDialog::Instance()->IsOk()) {
				std::string filePath = ImGuiFileDialog::Instance()->GetFilePathName();
				OpenScene(filePath);
			}
			ImGuiFileDialog::Instance()->Close();
		}

		if(ImGuiFileDialog::Instance()->Display("SceneCreateDialog")) {
			if(ImGuiFileDialog::Instance()->IsOk()) {
				CreateNewScene(ImGuiFileDialog::Instance()->GetFilePathName());
			}
			ImGuiFileDialog::Instance()->Close();
		}

		// ----------------------------
		// Save Scene ダイアログ処理
		// ----------------------------
		if(ImGuiFileDialog::Instance()->Display("SceneSaveDialog")) {
			if(ImGuiFileDialog::Instance()->IsOk()) {
				std::string filePath = ImGuiFileDialog::Instance()->GetFilePathName();
				if(ctx && SceneSerializer::Save(*ctx, filePath)) {
					ctx->SetScenePath(filePath);
					notifySceneSaved(filePath);
				} else {
					EngineLogger::GetInstance().Add(
						LogLevel::Error,
						LogCategory::Editor,
						"Failed to save scene: " + filePath,
						"LevelEditor");
				}
			}
			ImGuiFileDialog::Instance()->Close();
		}

		if(ImGuiFileDialog::Instance()->Display("PrefabOpenDialog")) {
			if(ImGuiFileDialog::Instance()->IsOk()) {
				std::string filePath = ImGuiFileDialog::Instance()->GetFilePathName();
				OpenPrefabForEdit(filePath);
			}
			ImGuiFileDialog::Instance()->Close();
		}

		if(ImGuiFileDialog::Instance()->Display("PrefabSaveAsDialog")) {
			if(ImGuiFileDialog::Instance()->IsOk()) {
				std::string filePath = ImGuiFileDialog::Instance()->GetFilePathName();
				SavePrefabEditAs(filePath);
			}
			ImGuiFileDialog::Instance()->Close();
		}

		// シーンコンテキスト切り替え検出
		NotifySceneContextChanged();
		prevCtx_ = SceneContext::Current();
		if(editToolMode_ == EngineEdit::EditToolMode::ParticleEffect && particlePreview_ && particlePreview_->Object()) {
			ActivateModeContext(editToolMode_);
			if(!selection_.HasSelection()) {
				SetSelectedObject(particlePreview_->Object());
			}
		} else if(editToolMode_ == EngineEdit::EditToolMode::Prefab && prefabEdit_ && prefabEdit_->Context()) {
			ActivateModeContext(editToolMode_);
		}

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

		// shortcutでのシーンの保存
		if(CalyxFoundation::Input::PushKey(DIK_LCONTROL)) {
			if(CalyxFoundation::Input::TriggerKey(DIK_S)) {
				if(SceneContext* scene = SceneContext::Current()) {
					const std::string scenePath = scene->GetScenePath();
					if(SceneSerializer::Save(*scene, scenePath)) {
						notifySceneSaved(scenePath);
					} else {
						EngineLogger::GetInstance().Add(
							LogLevel::Error,
							LogCategory::Editor,
							"Failed to save scene: " + scenePath,
							"LevelEditor");
					}
				}
			}
		}

		if(sceneSavedPopupTimer_ > 0.0f) {
			sceneSavedPopupTimer_ -= dt;
		}
		if(ImGui::BeginPopup("SceneSavedPopup")) {
			ImGui::TextUnformatted("シーンを保存しました");
			if(!sceneSavedPopupPath_.empty()) {
				ImGui::TextUnformatted(sceneSavedPopupPath_.c_str());
			}
			if(sceneSavedPopupTimer_ <= 0.0f) {
				ImGui::CloseCurrentPopup();
			}
			ImGui::EndPopup();
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
		SceneContext* previousContext = nullptr;
		if(editToolMode_ == EngineEdit::EditToolMode::ParticleEffect && particlePreview_ && particlePreview_->Context()) {
			previousContext = SceneContext::Current();
			particlePreview_->Context()->MakeCurrent();
		} else if(editToolMode_ == EngineEdit::EditToolMode::Prefab && prefabEdit_ && prefabEdit_->Context()) {
			previousContext = SceneContext::Current();
			prefabEdit_->Context()->MakeCurrent();
		}

		// 各パネル描画
		for(auto* p : editorPanels_) {
			if(p->IsShow()) {
				p->Render();
			}
		}
		editorToolRegistry_.Draw();

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

		if(previousContext && previousContext != SceneContext::Current()) {
			previousContext->MakeCurrent();
		}
	}

	//=============================================================================
	// Menu
	//=============================================================================
	void LevelEditor::RenderMenu() {
		if(menu_) {
			menu_->Render();
		}
		if(layoutSwitcher_ && ImGui::BeginMainMenuBar()) {
			layoutSwitcher_->DrawMenu();
			DrawEditModeCombo();
			ImGui::EndMainMenuBar();
		}
	}

	void LevelEditor::RenderSettingsWindow() {
		auto* settings = EngineSettings::GetInstance();
		settings->RenderSettingsWindow();
		if(sceneSettingsWindow_) {
			// SceneContext::CurrentはEditor mode切替時にも更新されるため、常に現在編集中のシーンを渡す。
			sceneSettingsWindow_->Render(SceneContext::Current());
		}
		if(settings->ConsumeApplied()) {
			if(auto* manipulator = sceneEditor_->GetManipulator()) {
				manipulator->ApplySettings(settings->GetData().manipulator);
			}
		}
	}

	void LevelEditor::DrawEditModeCombo() {
		ImGui::Separator();
		ImGui::TextUnformatted("EditMode");
		ImGui::SetNextItemWidth(140.0f);
		if(ImGui::BeginCombo("##EditToolMode", GetEditToolModeName(editToolMode_))) {
			if(layoutSwitcher_ && ImGui::BeginMenu("Object")) {
				for(const auto& preset : layoutSwitcher_->GetPresets()) {
					const bool selected = editToolMode_ == EngineEdit::EditToolMode::Object &&
										  preset.path == layoutSwitcher_->GetCurrentPath();
					if(ImGui::MenuItem(preset.name.c_str(), nullptr, selected)) {
						ApplyEditToolMode(EngineEdit::EditToolMode::Object, false);
						layoutSwitcher_->Apply(preset.path);
					}
				}
				ImGui::EndMenu();
			} else if(!layoutSwitcher_) {
				const bool selected = editToolMode_ == EngineEdit::EditToolMode::Object;
				if(ImGui::Selectable(GetEditToolModeName(EngineEdit::EditToolMode::Object), selected)) {
					ApplyEditToolMode(EngineEdit::EditToolMode::Object, false);
				}
			}

			const EngineEdit::EditToolMode modes[] = {
				EngineEdit::EditToolMode::Object2D,
				EngineEdit::EditToolMode::SpriteAnimation,
				EngineEdit::EditToolMode::Prefab,
				EngineEdit::EditToolMode::ParticleEffect,
				EngineEdit::EditToolMode::PostEffect,
				EngineEdit::EditToolMode::Material,
				EngineEdit::EditToolMode::Animation,
			};

			for(const auto mode : modes) {
				const bool selected = (mode == editToolMode_);
				if(ImGui::Selectable(GetEditToolModeName(mode), selected)) {
					ApplyEditToolMode(mode, true);
				}
				if(selected) ImGui::SetItemDefaultFocus();
			}
			ImGui::EndCombo();
		}
	}

	const char* LevelEditor::GetEditToolModeName(EngineEdit::EditToolMode mode) const {
		switch(mode) {
		case EngineEdit::EditToolMode::Object:
			return "Object";
		case EngineEdit::EditToolMode::Object2D:
			return "2D";
		case EngineEdit::EditToolMode::SpriteAnimation:
			return "UV Animation";
		case EngineEdit::EditToolMode::Prefab:
			return "Prefab";
		case EngineEdit::EditToolMode::ParticleEffect:
			return "Particle";
		case EngineEdit::EditToolMode::PostEffect:
			return "PostEffect";
		case EngineEdit::EditToolMode::Material:
			return "Material";
		case EngineEdit::EditToolMode::Animation:
			return "Animation";
		default:
			return "Object";
		}
	}

	std::string LevelEditor::GetEditToolModeLayoutPath(EngineEdit::EditToolMode mode) const {
		const std::filesystem::path layoutDir = Calyx::ResolveAssetPath("Configs/Editor/Layout");

		switch(mode) {
		case EngineEdit::EditToolMode::Object:
			return (layoutDir / "ObjectEdit.ini").generic_string();
		case EngineEdit::EditToolMode::Object2D:
			return (layoutDir / "Object2DEdit.ini").generic_string();
		case EngineEdit::EditToolMode::SpriteAnimation:
			return (layoutDir / "SpriteAnimationEdit.ini").generic_string();
		case EngineEdit::EditToolMode::Prefab:
			return (layoutDir / "PrefabEdit.ini").generic_string();
		case EngineEdit::EditToolMode::ParticleEffect:
			return (layoutDir / "ParticleEffectEdit.ini").generic_string();
		case EngineEdit::EditToolMode::PostEffect:
			return (layoutDir / "PostEfectEdit.ini").generic_string();
		case EngineEdit::EditToolMode::Material:
			return (layoutDir / "MaterialEdit.ini").generic_string();
		case EngineEdit::EditToolMode::Animation:
			return (layoutDir / "AnimationEdit.ini").generic_string();
		default:
			return (layoutDir / "ObjectEdit.ini").generic_string();
		}
	}

	std::string LevelEditor::GetEditToolModeLoadLayoutPath(EngineEdit::EditToolMode mode) const {
		const std::string modeLayoutPath = GetEditToolModeLayoutPath(mode);
		if(std::filesystem::exists(modeLayoutPath)) {
			return modeLayoutPath;
		}

		if(mode == EngineEdit::EditToolMode::Object || mode == EngineEdit::EditToolMode::Object2D) {
			return Calyx::ResolveAssetPath("Configs/Editor/Layout/gameEngineDefault.ini").generic_string();
		}

		return modeLayoutPath;
	}

	void LevelEditor::EnsureParticlePreviewContext() {
		if(particlePreview_) particlePreview_->Ensure();
	}

	void LevelEditor::UpdateParticlePreviewContext(float dt) {
		if(particlePreview_) particlePreview_->Update(dt);
	}

	void LevelEditor::EnsurePrefabEditContext() {
		if(prefabEdit_) prefabEdit_->Ensure();
	}

	void LevelEditor::NewPrefabEditContext(const std::string& rootTypeName) {
		if(!prefabEdit_) return;
		auto root = prefabEdit_->New(rootTypeName, sceneManager_);
		if(root) {
			SetSelectedObject(root);
		}
		NotifySceneContextChanged();
	}

	void LevelEditor::OpenPrefabForEdit(const std::string& path) {
		if(!prefabEdit_) return;
		auto selectedRoot = prefabEdit_->Open(path, sceneManager_);
		ApplyEditToolMode(EngineEdit::EditToolMode::Prefab, true);
		if(auto* context = prefabEdit_->Context()) {
			context->MakeCurrent();
		}
		if(selectedRoot) {
			SetSelectedObject(selectedRoot);
		}
		if(hierarchy_) hierarchy_->RefreshCache();

		NotifySceneContextChanged();
	}

	void LevelEditor::ApplyPrefabOverridesFromInstance(const std::shared_ptr<SceneObject>& object) {
		if(prefabEdit_ && prefabEdit_->ApplyOverridesFromInstance(object, sceneManager_)) {
			if(hierarchy_) hierarchy_->RefreshCache();
		}
	}

	void LevelEditor::SavePrefabEdit() {
		if(editToolMode_ != EngineEdit::EditToolMode::Prefab || !prefabEdit_ || !prefabEdit_->Context()) return;
		if(prefabEdit_->Path().empty()) {
			IGFD::FileDialogConfig config;
			config.path = Calyx::ResolveAssetPath("Prefabs").generic_string();
			ImGuiFileDialog::Instance()->OpenDialog(
				"PrefabSaveAsDialog",
				"save prefab file",
				".prefab",
				config);
			return;
		}

		if(prefabEdit_->Save(sceneManager_) && hierarchy_) {
			hierarchy_->RefreshCache();
		}
	}

	void LevelEditor::SavePrefabEditAs(const std::string& path) {
		if(editToolMode_ != EngineEdit::EditToolMode::Prefab || !prefabEdit_) return;
		if(prefabEdit_->SaveAs(path, sceneManager_) && hierarchy_) {
			hierarchy_->RefreshCache();
		}
	}

	void LevelEditor::UpdatePrefabEditContext(float dt) {
		if(prefabEdit_) prefabEdit_->Update(dt);
	}

	void LevelEditor::SaveActiveModeSelection() {
		modeSelections_[activeSelectionMode_] = selection_.Capture();
		selection_.ClearSceneContextSelection();
	}

	void LevelEditor::RestoreModeSelection(EngineEdit::EditToolMode mode) {
		auto it = modeSelections_.find(mode);
		if(it != modeSelections_.end()) {
			selection_.Restore(it->second);
			if(!it->second.selectedEditor) {
				selection_.PruneToContext(ResolveModeContext(mode));
			}
			return;
		} else {
			ClearSelection();
			if(mode == EngineEdit::EditToolMode::ParticleEffect &&
			   particlePreview_ &&
			   particlePreview_->Object()) {
				SetSelectedObject(particlePreview_->Object());
			}
		}

		selection_.PruneToContext(ResolveModeContext(mode));
	}

	SceneContext* LevelEditor::ResolveModeContext(EngineEdit::EditToolMode mode) const {
		switch(mode) {
		case EngineEdit::EditToolMode::ParticleEffect:
			return particlePreview_ ? particlePreview_->Context() : nullptr;
		case EngineEdit::EditToolMode::Prefab:
			return prefabEdit_ ? prefabEdit_->Context() : nullptr;
		default:
			return sceneManager_ ? sceneManager_->ActiveCtx() : SceneContext::Current();
		}
	}

	SceneContext* LevelEditor::ResolvePreviewContext(EngineEdit::EditToolMode mode) const {
		switch(mode) {
		case EngineEdit::EditToolMode::ParticleEffect:
			return particlePreview_ ? particlePreview_->Context() : nullptr;
		case EngineEdit::EditToolMode::Prefab:
			return prefabEdit_ ? prefabEdit_->Context() : nullptr;
		default:
			return nullptr;
		}
	}

	void LevelEditor::ActivateModeContext(EngineEdit::EditToolMode mode) {
		SceneContext* context = ResolveModeContext(mode);
		if(context) {
			context->MakeCurrent();
		}
		if(sceneManager_) {
			sceneManager_->SetEditorPreviewContext(ResolvePreviewContext(mode));
		}
	}

	void LevelEditor::ApplyEditToolMode(EngineEdit::EditToolMode mode, bool applyLayout) {
		const bool modeChanged = mode != editToolMode_;
		if(modeChanged) {
			SaveActiveModeSelection();
		}

		editToolMode_ = mode;

		auto setShow = [](IEngineUI* panel, bool show) {
			if(panel) {
				panel->SetShow(show);
			}
		};

		if(mainViewport_) mainViewport_->SetShow(true);
		if(debugViewport_) debugViewport_->SetShow(true);
		if(debugViewport_) debugViewport_->SetOverlayToolsEnabled(true);
		if(mainViewport_) mainViewport_->Set2DPlacementCanvasEnabled(false);

		switch(mode) {
		case EngineEdit::EditToolMode::Object:
			setShow(hierarchy_.get(), true);
			setShow(inspector_.get(), true);
			setShow(keyframePanel_.get(), false);
			setShow(placeToolPanel_.get(), true);
			setShow(splineEditor_.get(), false);
			setShow(assetPanel_.get(), true);
			setShow(materialNodeEditorPanel_.get(), false);
			setShow(postEffectNodeEditorPanel_.get(), false);
			setShow(spriteAnimationEditorPanel_.get(), false);
			break;
		case EngineEdit::EditToolMode::Object2D:
			if(mainViewport_) mainViewport_->SetShow(true);
			if(debugViewport_) debugViewport_->SetShow(false);
			if(mainViewport_) mainViewport_->SetOverlayToolsEnabled(true);
			if(mainViewport_) mainViewport_->Set2DPlacementCanvasEnabled(true);
			setShow(hierarchy_.get(), true);
			setShow(inspector_.get(), true);
			setShow(keyframePanel_.get(), true);
			setShow(placeToolPanel_.get(), false);
			setShow(splineEditor_.get(), false);
			setShow(assetPanel_.get(), true);
			setShow(materialNodeEditorPanel_.get(), false);
			setShow(postEffectNodeEditorPanel_.get(), false);
			setShow(spriteAnimationEditorPanel_.get(), false);
			break;
		case EngineEdit::EditToolMode::SpriteAnimation:
			if(mainViewport_) mainViewport_->SetShow(false);
			if(debugViewport_) debugViewport_->SetShow(false);
			setShow(hierarchy_.get(), false);
			setShow(inspector_.get(), false);
			setShow(keyframePanel_.get(), false);
			setShow(placeToolPanel_.get(), false);
			setShow(splineEditor_.get(), false);
			setShow(assetPanel_.get(), true);
			setShow(materialNodeEditorPanel_.get(), false);
			setShow(postEffectNodeEditorPanel_.get(), false);
			setShow(spriteAnimationEditorPanel_.get(), true);
			break;
		case EngineEdit::EditToolMode::Prefab:
			EnsurePrefabEditContext();
			if(mainViewport_) mainViewport_->SetShow(false);
			if(debugViewport_) debugViewport_->SetShow(true);
			if(debugViewport_) debugViewport_->SetOverlayToolsEnabled(true);
			setShow(hierarchy_.get(), true);
			setShow(inspector_.get(), true);
			setShow(keyframePanel_.get(), false);
			setShow(placeToolPanel_.get(), false);
			setShow(splineEditor_.get(), false);
			setShow(assetPanel_.get(), true);
			setShow(materialNodeEditorPanel_.get(), false);
			setShow(postEffectNodeEditorPanel_.get(), false);
			setShow(spriteAnimationEditorPanel_.get(), false);
			break;
		case EngineEdit::EditToolMode::ParticleEffect:
			EnsureParticlePreviewContext();
			if(mainViewport_) mainViewport_->SetShow(false);
			if(debugViewport_) debugViewport_->SetShow(true);
			if(debugViewport_) debugViewport_->SetOverlayToolsEnabled(true);
			setShow(hierarchy_.get(), true);
			setShow(inspector_.get(), true);
			setShow(keyframePanel_.get(), false);
			setShow(placeToolPanel_.get(), true);
			setShow(splineEditor_.get(), false);
			setShow(assetPanel_.get(), true);
			setShow(materialNodeEditorPanel_.get(), false);
			setShow(postEffectNodeEditorPanel_.get(), false);
			setShow(spriteAnimationEditorPanel_.get(), false);
			break;
		case EngineEdit::EditToolMode::PostEffect:
			setShow(hierarchy_.get(), false);
			setShow(inspector_.get(), false);
			setShow(keyframePanel_.get(), false);
			setShow(placeToolPanel_.get(), false);
			setShow(splineEditor_.get(), false);
			setShow(assetPanel_.get(), true);
			setShow(materialNodeEditorPanel_.get(), false);
			setShow(postEffectNodeEditorPanel_.get(), true);
			setShow(spriteAnimationEditorPanel_.get(), false);
			break;
		default:
			break;
		}

		ActivateModeContext(mode);
		if(modeChanged) {
			activeSelectionMode_ = mode;
			RestoreModeSelection(mode);
			NotifySceneContextChanged();
		} else {
			selection_.PruneToContext(ResolveModeContext(mode));
		}

		if(applyLayout && layoutSwitcher_) {
			layoutSwitcher_->Apply(GetEditToolModeLoadLayoutPath(mode));
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
		selection_.SetSelectedEditor(editor);
	}

	void LevelEditor::SetSelectedObject(const std::shared_ptr<SceneObject>& sp) {
		selection_.SetSelectedObject(sp);
	}

	void LevelEditor::ToggleSelectedObject(const std::shared_ptr<SceneObject>& sp) {
		selection_.ToggleSelectedObject(sp);
	}

	void LevelEditor::SetSelectedObjects(const std::vector<std::shared_ptr<SceneObject>>& objects) {
		selection_.SetSelectedObjects(objects);
	}

	bool LevelEditor::IsSelectedObject(const SceneObject* object) const {
		return selection_.IsSelected(object);
	}

	std::shared_ptr<SceneObject> LevelEditor::GetPrimarySelectedObject() const {
		return selection_.GetPrimarySelectedObject();
	}

	std::vector<std::shared_ptr<SceneObject>> LevelEditor::GetSelectedObjects() const {
		return selection_.GetSelectedObjects();
	}

	bool LevelEditor::IsPlaying() const {
		return pPlaySesseion_ && pPlaySesseion_->IsRuntime();
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
		obj->SetInstanceLifetime(ObjectInstanceLifetime::SceneOwned);
		ctx->AddObject(obj);
		EngineLogger::GetInstance().Add(
			LogLevel::Info,
			LogCategory::Editor,
			"Object created: " + obj->GetName(),
			"LevelEditor");

		if(hierarchy_) hierarchy_->RefreshCache();
	}

	void LevelEditor::DeleteObject(const std::shared_ptr<SceneObject>& sp) {
		if(!sp) return;

		SceneContext* ctx = SceneContext::Current();
		if(!ctx) return;

		// ── 選択状態をクリア（選択中だった場合） ─────────────────────
		if(IsSelectedObject(sp.get())) {
			ClearSelection();
		}

		CommandManager::GetInstance()->Execute(
			CreateDeleteSceneObjectsCommand(
				ctx,
				{sp},
				SceneObjectEditCommandCallbacks{
					[this]() {
						if(hierarchy_) hierarchy_->RefreshCache();
					},
					[this]() { ClearSelection(); },
					{}},
				"Delete Object"));

		EngineLogger::GetInstance().Add(
			LogLevel::Info,
			LogCategory::Editor,
			"Object deleted: " + sp->GetName(),
			"LevelEditor");
	}

	void LevelEditor::DeleteSelectedObjects() {
		std::vector<std::shared_ptr<SceneObject>> targets = GetSelectedObjects();

		if(targets.empty()) {
			ClearSelection();
			return;
		}

		SceneContext* ctx = SceneContext::Current();
		if(!ctx) return;

		CommandManager::GetInstance()->Execute(
			CreateDeleteSceneObjectsCommand(
				ctx,
				std::move(targets),
				SceneObjectEditCommandCallbacks{
					[this]() {
						if(hierarchy_) hierarchy_->RefreshCache();
					},
					[this]() { ClearSelection(); },
					{}},
				"Delete Selected Objects"));
	}

	std::vector<WorldTransform*> LevelEditor::DuplicateSelectedObjects() {
		std::vector<WorldTransform*> duplicatedTransforms;

		SceneContext* ctx = SceneContext::Current();
		if(!ctx) return duplicatedTransforms;

		auto duplicatableSources = SceneObjectDuplicator::FilterDuplicatable(GetSelectedObjects());
		if(duplicatableSources.empty()) return duplicatedTransforms;

		auto command = std::make_unique<DuplicateSceneObjectsCommand>(
			ctx,
			std::move(duplicatableSources),
			SceneObjectEditCommandCallbacks{
				[this]() {
					if(hierarchy_) hierarchy_->RefreshCache();
				},
				[this]() { ClearSelection(); },
				[this](const std::vector<std::shared_ptr<SceneObject>>& objects) {
					SetSelectedObjects(objects);
				}});
		auto* rawCommand = command.get();
		CommandManager::GetInstance()->Execute(std::move(command));

		const auto& createdObjects = rawCommand->GetCreatedRoots();
		duplicatedTransforms.reserve(createdObjects.size());
		for(const auto& object : createdObjects) {
			if(object) {
				duplicatedTransforms.push_back(&object->GetWorldTransform());
			}
		}
		return duplicatedTransforms;
	}

	//=============================================================================
	// Viewport
	//=============================================================================
	void LevelEditor::RenderViewport(ViewportType type, const ImTextureID& tex) {
		SceneContext* previousContext = nullptr;
		if(type == ViewportType::VIEWPORT_DEBUG) {
			if(SceneContext* previewContext = ResolvePreviewContext(editToolMode_)) {
				previousContext = SceneContext::Current();
				previewContext->MakeCurrent();
			}
		}

		auto restoreContext = [&]() {
			if(previousContext && previousContext != SceneContext::Current()) {
				previousContext->MakeCurrent();
			}
		};

		if(type == ViewportType::VIEWPORT_MAIN) {
			if(mainViewport_ && mainViewport_->IsShow()) {
				mainViewport_->Render(tex);
			}
		} else if(type == ViewportType::VIEWPORT_DEBUG) {
			if(debugViewport_ && debugViewport_->IsShow()) {
				const bool focusDebugViewport = startupDebugViewportFocusFrames_ > 0;
				if(focusDebugViewport) {
					ImGui::SetNextWindowFocus();
				}

				debugViewport_->Render(tex);

				if(focusDebugViewport) {
					ImGui::SetWindowFocus("Debug Viewport");
					--startupDebugViewportFocusFrames_;
				}

				if(viewportSelection_) viewportSelection_->DrawSelectionRect();
			}
		} else if(type == ViewportType::VIEWPORT_PICKING) {
			if(pickingViewport_ && pickingViewport_->IsShow()) {
				pickingViewport_->Render(tex);
			}
		} else if(type == ViewportType::VIEWPORT_PICKING_DEPTH) {
			if(pickingDepthViewport_ && pickingDepthViewport_->IsShow()) {
				pickingDepthViewport_->Render(tex);
			}
		}

		restoreContext();
	}

	void LevelEditor::RenderRuntimeFullscreenViewport(const ImTextureID& tex) {
		const ImGuiViewport* viewport = ImGui::GetMainViewport();
		const ImVec2 pos = viewport->WorkPos;
		const ImVec2 size = viewport->WorkSize;

		if(size.x <= 0.0f || size.y <= 0.0f) {
			return;
		}

		if(auto* mainCamera = CameraManager::GetMain3d()) {
			mainCamera->SetAspectRatio(size.x / size.y);
			mainCamera->UpdateMatrix();
			CameraManager::SetViewportSizeStatic(ViewportType::VIEWPORT_MAIN, {size.x, size.y});
		}

		ImGui::SetNextWindowPos(pos);
		ImGui::SetNextWindowSize(size);
		ImGui::SetNextWindowViewport(viewport->ID);

		const ImGuiWindowFlags flags =
			ImGuiWindowFlags_NoDecoration |
			ImGuiWindowFlags_NoMove |
			ImGuiWindowFlags_NoSavedSettings |
			ImGuiWindowFlags_NoBringToFrontOnFocus |
			ImGuiWindowFlags_NoScrollbar |
			ImGuiWindowFlags_NoScrollWithMouse;

		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
		if(ImGui::Begin("Runtime Game View", nullptr, flags)) {
			ImGui::Image(tex, ImGui::GetContentRegionAvail());
		}
		ImGui::End();
		ImGui::PopStyleVar();
	}

	bool LevelEditor::ShouldRenderRuntimeFullscreen() const {
		return pPlaySesseion_ &&
			   pPlaySesseion_->IsRuntime() &&
			   EngineSettings::GetInstance()->GetData().editor.fullscreenGameViewOnPlay;
	}

	bool LevelEditor::ShouldHideEditorUiInGameMode() const {
		return mode_ == EngineEdit::EditorMode::Game &&
			   EngineSettings::GetInstance()->GetData().editor.fullscreenGameViewOnPlay;
	}

	bool LevelEditor::IsDebugViewportVisible() const {
		return debugViewport_ && debugViewport_->IsShow();
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
	// Scene Save
	//=============================================================================
	void LevelEditor::SaveScene() {
		SceneContext* ctx = SceneContext::Current();
		if(!ctx) return;

		std::string scenePath = ctx->GetScenePath();
		if(scenePath.empty()) {
			scenePath = Calyx::ResolveAssetPath(std::filesystem::path("Scenes") / (ctx->GetSceneName() + ".scene")).generic_string();
		}
		if(SceneSerializer::Save(*ctx, scenePath)) {
			ctx->SetScenePath(scenePath);
			EngineLogger::GetInstance().Add(
				LogLevel::Info,
				LogCategory::Editor,
				"Scene saved: " + scenePath,
				"LevelEditor");
		} else {
			EngineLogger::GetInstance().Add(
				LogLevel::Error,
				LogCategory::Editor,
				"Failed to save scene: " + scenePath,
				"LevelEditor");
		}
	}

	//////////////////////////////////////////////////////////////////////////
	// CreateNewScene
	//////////////////////////////////////////////////////////////////////////
	bool LevelEditor::CreateNewScene(const std::filesystem::path& requestedDestination) {
		if(!sceneManager_) {
			EngineLogger::GetInstance().Add(LogLevel::Error, LogCategory::Editor, "Scene creation failed because SceneManager is unavailable.", "LevelEditor");
			return false;
		}

		// 保存前に拡張子と親ディレクトリを正規化し、AssetDatabaseへ同じパスで登録できる形にする。
		auto destination = NormalizeSceneCreatePath(requestedDestination);
		if(destination.empty() || !IsSceneFilePath(destination)) {
			EngineLogger::GetInstance().Add(LogLevel::Error, LogCategory::Editor, "Scene creation failed because the destination is not a .scene file.", "LevelEditor");
			return false;
		}

		std::error_code ec;
		const auto parentPath = destination.parent_path();
		if(!parentPath.empty()) {
			std::filesystem::create_directories(parentPath, ec);
			if(ec) {
				EngineLogger::GetInstance().Add(LogLevel::Error, LogCategory::Editor, "Scene directory could not be created: " + parentPath.generic_string() + " (" + ec.message() + ")", "LevelEditor");
				return false;
			}
		}

		const auto templatePath = FindDefaultSceneTemplate();
		if(templatePath.empty()) {
			EngineLogger::GetInstance().Add(LogLevel::Error, LogCategory::Editor, "Scene creation failed because DefaultScene.scene was not found.", "LevelEditor");
			return false;
		}

		// 既存のDirtyフラグ管理は未確認のため、現在シーンの保存確認はここでは行わない。
		// 新規シーンの初期データはDefaultSceneを正規読込し、テンプレート依存をSceneSerializer経由へ閉じ込める。
		SceneContext newSceneContext;
		newSceneContext.Initialize(false);
		if(!SceneSerializer::Load(newSceneContext, templatePath.generic_string())) {
			EngineLogger::GetInstance().Add(LogLevel::Error, LogCategory::Editor, "Scene creation failed because DefaultScene.scene could not be loaded: " + templatePath.generic_string(), "LevelEditor");
			return false;
		}
		RegenerateTemplateSceneGuids(newSceneContext);
		newSceneContext.SetSceneName(destination.stem().string());
		newSceneContext.SetScenePath(destination.generic_string());
		newSceneContext.SetSceneTransitionRequestor(&sceneManager_->GetTransitionRequestor());

		if(!SceneSerializer::Save(newSceneContext, destination.generic_string())) {
			EngineLogger::GetInstance().Add(LogLevel::Error, LogCategory::Editor, "Scene creation failed because the empty scene could not be saved: " + destination.generic_string(), "LevelEditor");
			return false;
		}

		if(auto* db = AssetDatabase::GetInstance()) {
			db->RegisterOrUpdate(destination, AssetType::Scene);
			db->Scan();
		}

		// 保存済みファイルを通常のシーン読込経路で開き、Hierarchy/Inspector/Viewport更新を同じ責務へ集約する。
		if(!OpenScene(destination)) return false;
		EngineLogger::GetInstance().Add(LogLevel::Info, LogCategory::Editor, "Scene created: " + destination.generic_string(), "LevelEditor");
		return true;
	}

	//////////////////////////////////////////////////////////////////////////
	// OpenScene
	//////////////////////////////////////////////////////////////////////////
	bool LevelEditor::OpenScene(const std::filesystem::path& path) {
		if(!sceneManager_) {
			EngineLogger::GetInstance().Add(LogLevel::Error, LogCategory::Editor, "Scene open failed because SceneManager is unavailable.", "LevelEditor");
			return false;
		}
		if(!IsSceneFilePath(path)) {
			EngineLogger::GetInstance().Add(LogLevel::Error, LogCategory::Editor, "Scene open failed because the path is not a .scene file: " + path.generic_string(), "LevelEditor");
			return false;
		}

		// 既存のDirtyフラグ管理は未確認のため、現在シーンの保存確認はここでは行わない。
		// 読込失敗時に現在のシーンを維持する処理はSceneManager::OpenSceneへ委譲する。
		if(!sceneManager_->OpenScene(path)) return false;

		// 読込成功後にEditor側の選択と各パネルのContext依存キャッシュを差し替える。
		ClearSelection();
		prevCtx_ = nullptr;
		NotifySceneContextChanged();
		return true;
	}

	bool LevelEditor::OpenSceneFromEditor(const std::filesystem::path& path) {
		return OpenScene(path);
	}

	//=============================================================================
	// SceneContext の変更検出
	//=============================================================================
	void LevelEditor::SetSceneManager(CalyxEngine::SceneManager* manager) {
		sceneManager_ = manager;
		auto context = editorToolRegistry_.GetContext();
		context.sceneManager = manager;
		editorToolRegistry_.SetContext(context);
		if(logPanel_) logPanel_->SetCommandContext({this, sceneManager_, pPlaySesseion_});

		if(manager) {
			auto* pickingPass = manager->GetPickingPass();
			if(mainViewport_) mainViewport_->SetPickingPass(pickingPass);
			if(debugViewport_) debugViewport_->SetPickingPass(pickingPass);
			if(pickingViewport_) pickingViewport_->SetPickingPass(pickingPass);
			if(pickingDepthViewport_) pickingDepthViewport_->SetPickingPass(pickingPass);
		}
		if(viewportSelection_) viewportSelection_->SetSceneManager(manager);
		if(manager && editToolMode_ == EngineEdit::EditToolMode::ParticleEffect) {
			manager->SetEditorPreviewContext(particlePreview_ ? particlePreview_->Context() : nullptr);
		}

		if(sceneSwitchOverlay_) {
			sceneSwitchOverlay_->SetSceneManager(manager);
		}
	}

	void LevelEditor::SetPlaySession(PlaySession* session) {
		pPlaySesseion_ = session;
		if(logPanel_) logPanel_->SetCommandContext({this, sceneManager_, pPlaySesseion_});
	}
	void LevelEditor::NotifySceneContextChanged() {
		SceneContext* current = SceneContext::Current();
		if(prevCtx_ == current) return;

		if(hierarchy_) {
			hierarchy_->SetSceneObjectLibrary(current ? current->GetObjectLibrary() : nullptr);
			hierarchy_->RefreshCache();
		}

		selection_.PruneToContext(current);

		if(current) {
			current->AddOnObjectRemovedListener(
				[editor = this](SceneObject* removed) {
					if(!editor) return;

					if(editor->IsSelectedObject(removed)) {
						editor->ClearSelection();
					}
				});

			if(sceneEditor_) {
				sceneEditor_->BindRemovalCallback(current);
			}
		}
	}
	void LevelEditor::ClearSelection() {
		selection_.Clear();
	}

} // namespace CalyxEngine
