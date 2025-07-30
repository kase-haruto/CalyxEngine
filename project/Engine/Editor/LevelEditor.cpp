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

#include <externals/imgui/ImGuiFileDialog.h>

using namespace EngineEdit;

void LevelEditor::Initialize(){
#ifdef _DEBUG
	// 各パネルの初期化
	hierarchy_ = std::make_unique<HierarchyPanel>();
	editor_ = std::make_unique<EditorPanel>();
	inspector_ = std::make_unique<InspectorPanel>();
	sceneEditor_ = std::make_unique<SceneObjectEditor>();
	placeToolPanel_ = std::make_unique<PlaceToolPanel>();

	// Panel に LevelEditor 自体を渡す（コールバック通知や setter）
	editor_->SetOnEditorSelected([this] (BaseEditor* ed){ SetSelectedEditor(ed); });

	// Hierarchy から来るコールバックは shared_ptr で受ける
	hierarchy_->SetOnObjectSelected([this] (std::shared_ptr<SceneObject> sp){
		SetSelectedObject(sp);
									});
	hierarchy_->SetOnObjectDelete([this] (std::shared_ptr<SceneObject> sp){
		DeleteObject(std::move(sp));           // 参照カウント‐1
								  });
	hierarchy_->SetOnObjectCreate([this] (std::shared_ptr<SceneObject> sp){
		CreateObject(std::move(sp));           // 受け取ってシーンに追加
								  });

	inspector_->SetSceneObjectEditor(sceneEditor_.get());

	// ビューポートの初期化
	mainViewport_ = std::make_unique<Viewport>(ViewportType::VIEWPORT_MAIN, "Game Viewport");
	debugViewport_ = std::make_unique<Viewport>(ViewportType::VIEWPORT_DEBUG, "Debug Viewport");

	performanceOverlay_ = std::make_unique<PerformanceOverlay>();

	// Manipulator をツールとして登録
	if (auto* manipulator = sceneEditor_->GetManipulator()){
		debugViewport_->AddTool(manipulator);
		debugViewport_->AddTool(performanceOverlay_.get());
	}

	// エディターメニューの初期化
	menu_ = std::make_unique<EditorMenu>();
	menu_->Add(MenuCategory::File, {
		"Save Scene", "Ctrl+S", [this] (){
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
	"Open Scene", "Ctrl+O", []{
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

#endif // _DEBUG
}

void LevelEditor::Update(){

#ifdef _DEBUG
	SceneContext* ctx = SceneContext::Current();

	// 入力チェックはここで行う
	if (Input::GetInstance()->TriggerMouseButton(MouseButton::Left)){
		TryPickUnderCursor(); // レイキャストして選択処理へ
	}
	// ----------------------------
	// Open Scene ダイアログ処理
	// ----------------------------
	if (ImGuiFileDialog::Instance()->Display("SceneOpenDialog")){
		if (ImGuiFileDialog::Instance()->IsOk()){
			std::string filePath = ImGuiFileDialog::Instance()->GetFilePathName();
			ClearSelection(); // 既存の選択をクリア
			SceneSerializer::Load(*ctx, filePath);
		}
		ImGuiFileDialog::Instance()->Close();
	}

	// ----------------------------
	// Save Scene ダイアログ処理
	// ----------------------------
	if (ImGuiFileDialog::Instance()->Display("SceneSaveDialog")){
		if (ImGuiFileDialog::Instance()->IsOk()){
			std::string filePath = ImGuiFileDialog::Instance()->GetFilePathName();
			SceneSerializer::Save(*ctx, filePath);
		}
		ImGuiFileDialog::Instance()->Close();
	}

	//シーンが変わっていたら各パネルに通知する
	NotifySceneContextChanged();
	prevCtx_ = SceneContext::Current();
#endif // _DEBUG
}

void LevelEditor::Render(){

	hierarchy_->Render();
	editor_->Render();
	placeToolPanel_->Render();
	if (pPlaySesseion_){
		pPlaySesseion_->RenderToolbar();
	}

	inspector_->SetSelectedEditor(selectedEditor_);
	inspector_->SetSelectedObject(selectedObject_);

	inspector_->Render();

	sceneEditor_->Update();
}

void LevelEditor::RenderMenu() {
	menu_->Render();
}

void LevelEditor::EnterGameMode(){
	//playSession_->Enter();
	mode_ = EditorMode::Game;
}

void LevelEditor::ExitGameMode(){
	//playSession_->Exit();
	mode_ = EditorMode::Edit;
}

void LevelEditor::SetSelectedEditor(BaseEditor* editor){
	selectedEditor_ = editor;
	selectedObject_ = nullptr;
	inspector_->SetSelectedEditor(editor);
}

void LevelEditor::SetSelectedObject(const std::shared_ptr<SceneObject>& sp){
	selectedObject_ = sp;
	selectedEditor_ = nullptr;
	hierarchy_->SetSelectedObject(sp);
	inspector_->SetSelectedObject(sp);

}

void LevelEditor::CreateObject(std::shared_ptr<SceneObject> obj){
	if (!obj) return;
	SceneContext* ctx = SceneContext::Current();

	// パーティクルなら FxSystem 登録
	if (obj->GetObjectType() == ObjectType::ParticleSystem){
		if (auto fx = std::dynamic_pointer_cast< ParticleSystemObject >(obj)){
			ctx->GetFxSystem()->AddEmitter(fx);
		}
	}
	ctx->GetObjectLibrary()->AddObject(obj);
}


void LevelEditor::DeleteObject(const std::shared_ptr<SceneObject>& sp){
	if (!sp) return;
	SceneContext* ctx = SceneContext::Current();

	// ── 選択状態をクリア ───────────────────────────────
	if (selectedObject_ == sp){
		selectedObject_.reset();
		inspector_->SetSelectedObject(nullptr);
	}

	// ── パーティクルシステムなら FxSystem からも削除 ──────────
	if (sp->GetObjectType() == ObjectType::ParticleSystem){
		if (auto fxEmitter = std::dynamic_pointer_cast< FxEmitter >(sp)){
			ctx->GetFxSystem()->RemoveEmitter(fxEmitter.get());
		}
	}

	// ── シーンから除去 ────────────────────────────────
	ctx->RemoveEditorObject(sp);          // editorObjects_ から
	ctx->GetObjectLibrary()->RemoveObject(sp); // ライブラリから
}


void LevelEditor::RenderViewport(ViewportType type, const ImTextureID& tex){
	//タイプに応じて描画
	if (type == ViewportType::VIEWPORT_MAIN){
		if (mainViewport_){
			mainViewport_->Render(tex);
		}
	} else if (type == ViewportType::VIEWPORT_DEBUG){
		if (debugViewport_){
			debugViewport_->Render(tex);
		}
	}
}

void LevelEditor::SetCameraForViewport(BaseCamera* mainCamera, BaseCamera* debugCamera){
	mainViewport_->SetCamera(mainCamera);
	debugViewport_->SetCamera(debugCamera);
}


void LevelEditor::TryPickObjectFromMouse(const Vector2& mouse,
										 const Vector2& viewportSize,
										 const Matrix4x4& view,
										 const Matrix4x4& proj){
	SceneContext* ctx = SceneContext::Current();

	// ビューポート内ローカル座標へ変換
	Vector2 mouseLocal = mouse - debugViewport_->GetPosition();

	Ray ray = Raycastor::ConvertMouseToRay(mouseLocal, view, proj, viewportSize);

	// ヒット判定（raw ptr）
	if (SceneObject* raw = PickSceneObjectByRay(ray)){
		// 対応する shared_ptr をライブラリから取得
		if (auto sp = ctx->FindSharedObject(raw)){
			SetSelectedObject(sp);
		}
	}
}


SceneObject* LevelEditor::PickSceneObjectByRay(const Ray& ray){
	const auto* lib = hierarchy_->GetSceneObjectLibrary();
	if (!lib) return nullptr;

	const auto& allObjects = lib->GetAllObjectsRaw();
	auto hit = Raycastor::Raycast(ray, allObjects);
	if (hit){
		return static_cast< SceneObject* >(hit->hitObject);
	}
	return nullptr;
}

void LevelEditor::SaveScene(){
	SceneContext* ctx = SceneContext::Current();

	std::string scenePath = "Resources/Assets/Scenes/" + ctx->GetSceneName() + ".scene";
	SceneSerializer::Save(*ctx, scenePath);
}

void LevelEditor::NotifySceneContextChanged(){
	if (prevCtx_!=SceneContext::Current()) {
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


void LevelEditor::TryPickUnderCursor(){
	SceneContext* current = SceneContext::Current();

	Vector2 origin = debugViewport_->GetPosition();	// ビューポート描画位置（スクリーン座標）
	Vector2 size = debugViewport_->GetSize();		// ビューポートの実際のサイズ（ピクセル）

	// ImGui上のマウス位置
	ImVec2 mouse = ImGui::GetMousePos();

	// ビューポート内のローカルマウス座標
	float relativeX = mouse.x - origin.x;
	float relativeY = mouse.y - origin.y;

	// 範囲外なら無視
	if (relativeX < 0 || relativeY < 0 || relativeX > size.x || relativeY > size.y) return;

	// スクリーン→ビューポート空間（ピクセル）
	Vector2 mousePos = Vector2(relativeX, relativeY);

	// Ray生成
	Matrix4x4 view = CameraManager::GetDebug()->GetViewMatrix();
	Matrix4x4 proj = CameraManager::GetDebug()->GetProjectionMatrix();

	Ray ray = Raycastor::ConvertMouseToRay(mousePos, view, proj, size);
	if (SceneObject* picked = PickSceneObjectByRay(ray)){
		if (auto sp = current->FindSharedObject(picked)){
			// 共有ポインタを渡す
			SetSelectedObject(sp);
		}
	}
}