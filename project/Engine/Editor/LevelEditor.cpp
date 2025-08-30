#include "LevelEditor.h"
// engine
#include <Engine/Application/UI/EngineUI/Context/EditorContext.h>
#include <Engine/Application/Input/Input.h>
#include <Engine/Graphics/Camera/Manager/CameraManager.h>
#include <Engine/Physics/Ray/Raycastor.h>
#include <Engine/Objects/3D/Actor/Library/SceneObjectLibrary.h>
#include <Engine/Application/Effects/FxSystem.h>
#include <Engine/Scene/Context/SceneContext.h>
#include <Engine/Scene/Serializer/SceneSerializer.h>
#include <Engine/Application/Effects/Particle/Object/ParticleSystemObject.h>
#include <Engine/Application/System/PlaySession.h>
#include <Engine/Assets/Database/AssetDatabase.h>

#include <externals/imgui/ImGuiFileDialog.h>

using namespace EngineEdit;

void LevelEditor::Initialize() {
#ifdef _DEBUG
	// 各パネルの初期化
	hierarchy_ = std::make_unique<HierarchyPanel>();
	editor_ = std::make_unique<EditorPanel>();
	inspector_ = std::make_unique<InspectorPanel>();
	sceneEditor_ = std::make_unique<SceneObjectEditor>();
	placeToolPanel_ = std::make_unique<PlaceToolPanel>();
	splineEditor_ = std::make_unique<SplineEditorPanel>();
	assetPanel_ = std::make_unique<AssetPanel>();
	auto* db = AssetDatabase::GetInstance();
	assetPanel_->Initialize(db->GetRoot());

	// Panel に LevelEditor 自体を渡す（コールバック通知や setter）
	editor_->SetOnEditorSelected([this](BaseEditor* ed) { SetSelectedEditor(ed); });

	// Hierarchy から来るコールバックは shared_ptr で受ける
	hierarchy_->SetOnObjectSelected([this](std::shared_ptr<SceneObject> sp) {
		SetSelectedObject(sp);
	});
	hierarchy_->SetOnObjectDelete([this](std::shared_ptr<SceneObject> sp) {
		DeleteObject(std::move(sp));
	});
	hierarchy_->SetOnObjectCreate([this](std::shared_ptr<SceneObject> sp) {
		CreateObject(std::move(sp));
	});

	inspector_->SetSceneObjectEditor(sceneEditor_.get());

	// ビューポートの初期化
	mainViewport_ = std::make_unique<Viewport>(ViewportType::VIEWPORT_MAIN, "Game Viewport");
	mainViewport_->SetShow(false);
	debugViewport_ = std::make_unique<Viewport>(ViewportType::VIEWPORT_DEBUG, "Debug Viewport");

	performanceOverlay_ = std::make_unique<PerformanceOverlay>();

	// Manipulator をツールとして登録
	if (auto* manipulator = sceneEditor_->GetManipulator()) {
		debugViewport_->AddTool(manipulator);
		debugViewport_->AddTool(performanceOverlay_.get());
	}

	// エディターメニューの初期化
	menu_ = std::make_unique<EditorMenu>();
	menu_->Add(MenuCategory::File, {
		"Save Scene", "Ctrl+S", [this]() {
			IGFD::FileDialogConfig config;
			config.path = "Resources/Assets/Scenes/";
			ImGuiFileDialog::Instance()->OpenDialog(
				"SceneSaveDialog",
				"load scene file",
				".scene",
				config
			);
		}, true
			   });

	menu_->Add(MenuCategory::File, {
		"Open Scene", "Ctrl+O", [] {
			IGFD::FileDialogConfig config;
			config.path = "Resources/Assets/Scenes/";
			ImGuiFileDialog::Instance()->OpenDialog(
				"SceneOpenDialog",
				"open scene",
				".scene",
				config
			);
		}, true
			   });

	if (mode_ == EditorMode::Edit) {
		menu_->Add(MenuCategory::View, {
			"Enter Game Mode", "", [this]() { ToggleMode(); }, true
				   });
	} else {
		menu_->Add(MenuCategory::View, {
			"Exit Game Mode", "", [this]() { ToggleMode(); }, true
				   });
	}

	// パネル群を登録（Editors メニューに並べる）
	editorPanels_.push_back(hierarchy_.get());
	editorPanels_.push_back(editor_.get());
	editorPanels_.push_back(inspector_.get());
	editorPanels_.push_back(placeToolPanel_.get());
	editorPanels_.push_back(splineEditor_.get());
	editorPanels_.push_back(assetPanel_.get());

	// Editors メニュー（MenuCategory::Tools）に各パネルのトグルを追加
	for (auto* p : editorPanels_) {
		menu_->Add(MenuCategory::Tools, {
			p->GetPanelName(), "",
			[p, this]() {
				TogglePanel(p); // 表示/非表示トグル
			},
			true
				   });
	}

	menu_->Add(MenuCategory::View, { "Main Viewport",  "", [this] { mainViewport_->SetShow(!mainViewport_->IsShow());     }, true });
	menu_->Add(MenuCategory::View, { "Debug Viewport", "", [this] { debugViewport_->SetShow(!debugViewport_->IsShow());   }, true });

	menu_->Add(MenuCategory::Edit, { "Play ", "(F5)", [this] {
	if (pPlaySesseion_ && !pPlaySesseion_->IsRuntime()) {
		pPlaySesseion_->Enter();
	}
	}, true });

	menu_->Add(MenuCategory::Edit, { "Pause ", "(F6)", [this] {
		if (pPlaySesseion_ && pPlaySesseion_->IsRuntime()) {
			pPlaySesseion_->TogglePause();
		}
	}, true });

	menu_->Add(MenuCategory::Edit, { "Exit ", "(Shift+F5)", [this] {
		if (pPlaySesseion_ && pPlaySesseion_->IsRuntime()) {
			pPlaySesseion_->Exit();
		}
	}, true });
#endif // _DEBUG
}

void LevelEditor::Update() {
#ifdef _DEBUG
	SceneContext* ctx = SceneContext::Current();

	const ImGuiIO& io = ImGui::GetIO();
	const bool guizmoActive = ImGuizmo::IsOver() || ImGuizmo::IsUsing();

	// --- デバッグビューポート上にマウスがあるか？ ---
	bool overDebugViewport = false;
	if (debugViewport_ && debugViewport_->IsShow()) {
		const Vector2 origin = debugViewport_->GetPosition(); // コンテンツ左上
		const Vector2 size = debugViewport_->GetSize();     // コンテンツサイズ
		const ImVec2  mouse = ImGui::GetMousePos();

		overDebugViewport =
			(mouse.x >= origin.x && mouse.y >= origin.y &&
			 mouse.x <= origin.x + size.x && mouse.y <= origin.y + size.y);
	}

	// ImGui がマウスを掴んでいても、ビューポート上なら許可
	const bool uiBlocksClick = io.WantCaptureMouse && !overDebugViewport;

	if (debugViewport_ && debugViewport_->IsShow() && !guizmoActive && !uiBlocksClick) {
		// ★ ImGui と DirectInput の両方で“立ち上がり”を検出（更新順の影響を低減）
		const bool imguiEdge = ImGui::IsMouseClicked(ImGuiMouseButton_Left);

		static bool prevDILeft = false;
		const bool diNow = Input::GetInstance()->PushMouseButton(MouseButton::Left);
		const bool diEdge = diNow && !prevDILeft;
		prevDILeft = diNow;

		if (imguiEdge || diEdge) {
			TryPickUnderCursor();
		}
	}

	// ----------------------------
	// Open Scene ダイアログ処理
	// ----------------------------
	if (ImGuiFileDialog::Instance()->Display("SceneOpenDialog")) {
		if (ImGuiFileDialog::Instance()->IsOk()) {
			std::string filePath = ImGuiFileDialog::Instance()->GetFilePathName();
			ClearSelection();
			SceneSerializer::Load(*ctx, filePath);
		}
		ImGuiFileDialog::Instance()->Close();
	}

	// ----------------------------
	// Save Scene ダイアログ処理
	// ----------------------------
	if (ImGuiFileDialog::Instance()->Display("SceneSaveDialog")) {
		if (ImGuiFileDialog::Instance()->IsOk()) {
			std::string filePath = ImGuiFileDialog::Instance()->GetFilePathName();
			SceneSerializer::Save(*ctx, filePath);
		}
		ImGuiFileDialog::Instance()->Close();
	}

	NotifySceneContextChanged();
	prevCtx_ = SceneContext::Current();

	if (pPlaySesseion_) {
		const bool playing = pPlaySesseion_->IsRuntime();
		if (playing && !lastPlaying_) EnterGameMode();
		else if (!playing && lastPlaying_) ExitGameMode();
		lastPlaying_ = playing;
	}
#endif
}


void LevelEditor::Render() {

	for (auto* p : editorPanels_) {
		if (p->IsShow()) p->Render();
	}

	if (pPlaySesseion_) {
		pPlaySesseion_->RenderToolbar();
	}

	inspector_->SetSelectedEditor(selectedEditor_);
	inspector_->SetSelectedObject(selectedObject_);

	// inspector は editorPanels_ に含まれているので、ここで Render は呼ばない
	// inspector_->Render();

	sceneEditor_->Update();
}

void LevelEditor::RenderMenu() {
	menu_->Render();
}

void LevelEditor::EnterGameMode() {
	mode_ = EditorMode::Game;
}

void LevelEditor::ExitGameMode() {
	mode_ = EditorMode::Edit;

	if (pPlaySesseion_ && pPlaySesseion_->IsRuntime()) {
		pPlaySesseion_->Exit();
		lastPlaying_ = false;
	}
}

void LevelEditor::SetSelectedEditor(BaseEditor* editor) {
	selectedEditor_ = editor;
	selectedObject_ = nullptr;
	inspector_->SetSelectedEditor(editor);
}

void LevelEditor::SetSelectedObject(const std::shared_ptr<SceneObject>& sp) {
	selectedObject_ = sp;
	selectedEditor_ = nullptr;
	hierarchy_->SetSelectedObject(sp);
	inspector_->SetSelectedObject(sp);

}
void LevelEditor::CreateObject(const std::shared_ptr<SceneObject>& obj) {
	if (!obj) return;

	SceneContext* ctx = SceneContext::Current();

	// パーティクルなら FxSystem にも登録
	if (obj->GetObjectType() == ObjectType::ParticleSystem) {
		if (auto fx = std::dynamic_pointer_cast<ParticleSystemObject>(obj)) {
			ctx->GetFxSystem()->AddEmitter(fx);
		}
	}

	// 自身を登録
	ctx->GetObjectLibrary()->AddObject(obj);

	// 子も再帰的に登録
	for (const auto& child : obj->GetChildren()) {
		if (child) {
			CreateObject(child);
		}
	}
}


void LevelEditor::DeleteObject(const std::shared_ptr<SceneObject>& sp) {
	if (!sp) return;
	SceneContext* ctx = SceneContext::Current();

	// ── 選択状態をクリア ───────────────────────────────
	if (selectedObject_ == sp) {
		selectedObject_.reset();
		inspector_->SetSelectedObject(nullptr);
	}

	// ── パーティクルシステムなら FxSystem からも削除 ──────────
	if (sp->GetObjectType() == ObjectType::ParticleSystem) {
		if (auto fxEmitter = std::dynamic_pointer_cast<FxEmitter>(sp)) {
			ctx->GetFxSystem()->RemoveEmitter(fxEmitter.get());
		}
	}

	// ── シーンから除去 ────────────────────────────────
	ctx->RemoveEditorObject(sp);          // editorObjects_ から
	ctx->GetObjectLibrary()->RemoveObject(sp); // ライブラリから
}

void LevelEditor::RenderViewport(ViewportType type, const ImTextureID& tex) {
	if (type == ViewportType::VIEWPORT_MAIN) {
		if (mainViewport_ && mainViewport_->IsShow()) {   // ★ 追加
			mainViewport_->Render(tex);
		}
	} else if (type == ViewportType::VIEWPORT_DEBUG) {
		if (debugViewport_ && debugViewport_->IsShow()) {  // ★ 追加
			debugViewport_->Render(tex);
		}
	}
}

void LevelEditor::SetCameraForViewport(BaseCamera* mainCamera, BaseCamera* debugCamera) {
	mainViewport_->SetCamera(mainCamera);
	debugViewport_->SetCamera(debugCamera);
}


void LevelEditor::TryPickObjectFromMouse(const Vector2& mouse,
										 const Vector2& viewportSize,
										 const Matrix4x4& view,
										 const Matrix4x4& proj) {
	SceneContext* ctx = SceneContext::Current();

	// ビューポート内ローカル座標へ変換
	Vector2 mouseLocal = mouse - debugViewport_->GetPosition();

	Ray ray = Raycastor::ConvertMouseToRay(mouseLocal, view, proj, viewportSize);

	// ヒット判定（raw ptr）
	if (SceneObject* raw = PickSceneObjectByRay(ray)) {
		// 対応する shared_ptr をライブラリから取得
		if (auto sp = ctx->FindSharedObject(raw)) {
			SetSelectedObject(sp);
		}
	}
}


SceneObject* LevelEditor::PickSceneObjectByRay(const Ray& ray) {
	const auto* lib = hierarchy_->GetSceneObjectLibrary();
	if (!lib) return nullptr;

	const auto& allObjects = lib->GetAllObjectsRaw();
	auto hit = Raycastor::Raycast(ray, allObjects);
	if (hit) {
		return static_cast<SceneObject*>(hit->hitObject);
	}
	return nullptr;
}

void LevelEditor::SaveScene() {
	SceneContext* ctx = SceneContext::Current();

	std::string scenePath = "Resources/Assets/Scenes/" + ctx->GetSceneName() + ".scene";
	SceneSerializer::Save(*ctx, scenePath);
}

void LevelEditor::NotifySceneContextChanged() {
	if (prevCtx_ != SceneContext::Current()) {
		SceneContext* current = SceneContext::Current();

		// 新しい ObjectLibrary を各パネルに通知
		hierarchy_->SetSceneObjectLibrary(current ? current->GetObjectLibrary() : nullptr);

		SetSelectedObject(std::shared_ptr<SceneObject>{});
		ClearSelection();

		if (current) {
			current->AddOnObjectRemovedListener(
				[editor = this](SceneObject* removed) {
				auto sel = editor->GetHierarchyPanel()->GetSelectedObject();
				if (sel && sel.get() == removed) {
					editor->SetSelectedObject(std::shared_ptr<SceneObject>{});
				}
			}
			);

			// SceneObjectEditor: 削除されたら無効化
			sceneEditor_->BindRemovalCallback(current);
		}
	}
}

void LevelEditor::ToggleMode() {
	if (mode_ == EditorMode::Edit) {
		mode_ = EditorMode::Game;
	} else {
		mode_ = EditorMode::Edit;
	}
}

void LevelEditor::TryPickUnderCursor() {
	if (!debugViewport_ || !debugViewport_->IsShow()) return;

	SceneContext* current = SceneContext::Current();
	Vector2 origin = debugViewport_->GetPosition();
	Vector2 size = debugViewport_->GetSize();

	ImVec2 mouse = ImGui::GetMousePos();
	float relativeX = mouse.x - origin.x;
	float relativeY = mouse.y - origin.y;

	if (relativeX < 0 || relativeY < 0 || relativeX > size.x || relativeY > size.y) return;

	Vector2 mousePos = Vector2(relativeX, relativeY);

	Matrix4x4 view = CameraManager::GetDebug()->GetViewMatrix();
	Matrix4x4 proj = CameraManager::GetDebug()->GetProjectionMatrix();

	Ray ray = Raycastor::ConvertMouseToRay(mousePos, view, proj, size);
	if (SceneObject* picked = PickSceneObjectByRay(ray)) {
		if (auto sp = current->FindSharedObject(picked)) {
			SetSelectedObject(sp);
		}
	}
}
