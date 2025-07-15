#include "EngineController.h"
/* ========================================================================
/*	include space
/* ===================================================================== */
// engine
#include <Engine/Application/System/Enviroment.h>
#include <Engine/Scene/System/SceneManager.h>
#include <Engine/Application/Input/Input.h>
#include <Engine/Application/UI/EngineUI/Core/EngineUICore.h>
#include <Engine/Editor/PostProcessEditor.h>
#include <Engine/Foundation/Clock/ClockManager.h>
#include <Engine/Application/Effects/FxSystem.h>

/////////////////////////////////////////////////////////////////////////////////////////
//		engine 初期化
/////////////////////////////////////////////////////////////////////////////////////////
void EngineController::Initialize(HINSTANCE hInstance){
	// comの初期化
	CoInitializeEx(0, COINIT_MULTITHREADED);

	// engineの初期化
	system_ = std::make_unique<System>();
	system_->Initialize(hInstance, kWindowWidth, kWindowHeight, windowTitle);

	// graphicsシステムの初期化
	graphicsSystem_ = std::make_unique<GraphicsSystem>();
	graphicsSystem_->Initialize();

	system_->InitializePostProcess(graphicsSystem_->GetPipelineService());

	// engineUIの初期化
	engineUICore_ = std::make_unique<EngineUICore>();
	engineUICore_->Initialize();
	system_->SetEngineUICore(engineUICore_.get());

	// engineEditorの初期化
	editorCollection_ = std::make_unique<EditorCollection>();
	editorCollection_->InitializeEditors();
	auto ppCollection = dynamic_cast< PostProcessEditor* >(editorCollection_->GetEditor(EditorCollection::EditorType::PostProcess));
	ppCollection->SetPostEffectCollection(system_->GetPostProcessCollection());

	// エディターパネルにエディターを追加
#ifdef _DEBUG
	engineUICore_->GetEditorPanel()->AddEditor(editorCollection_->GetEditor(EditorCollection::EditorType::PostProcess));
#endif // _DEBUG


	// シーンマネージャ初期化
	sceneManager_ = std::make_unique<SceneManager>(system_->GetDxCore(),graphicsSystem_.get());
	sceneManager_->SetEngineUI(engineUICore_.get());
	sceneManager_->Initialize();

}

/////////////////////////////////////////////////////////////////////////////////////////
//		メインループ
/////////////////////////////////////////////////////////////////////////////////////////
void EngineController::Run(){
	// メインループ
	while (!system_->ProcessMessage()){

		//更新処理
		Update();

		//描画処理
		Render();

		if (Input::TriggerKey(DIK_ESCAPE)){
			break;
		}
	}
}

/////////////////////////////////////////////////////////////////////////////////////////
//		終了処理
/////////////////////////////////////////////////////////////////////////////////////////
void EngineController::Finalize(){
	//終了処理
	system_->Finalize();
	CoUninitialize();
}

/////////////////////////////////////////////////////////////////////////////////////////
//		更新
/////////////////////////////////////////////////////////////////////////////////////////
bool EngineController::Update() {
	BeginUpdate();

	// シーンの更新
	sceneManager_->Update();

	EndUpdate();

	return true;
}

/////////////////////////////////////////////////////////////////////////////////////////
//		更新前処理
/////////////////////////////////////////////////////////////////////////////////////////
void EngineController::BeginUpdate(){
	// 描画前処理
	system_->BeginFrame();

	engineUICore_->Update();
}

/////////////////////////////////////////////////////////////////////////////////////////
//		更新後処理
/////////////////////////////////////////////////////////////////////////////////////////
void EngineController::EndUpdate(){
	// UI描画
	engineUICore_->Render();	

}


/////////////////////////////////////////////////////////////////////////////////////////
//		描画
/////////////////////////////////////////////////////////////////////////////////////////
void EngineController::Render() {
	// シーンの描画
	sceneManager_->Draw();

	// 描画後処理
	system_->EndFrame(graphicsSystem_->GetPipelineService());
}

