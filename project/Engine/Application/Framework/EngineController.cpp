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

	postProcessCollection_ = system_->GetPostProcessCollection();
	postEffectSlots_ = {
		{ "RadialBlur", false, postProcessCollection_->GetEffectByName("RadialBlur") },
		{ "GrayScale",  false,  postProcessCollection_->GetEffectByName("GrayScale")},
		{ "CopyImage",  true,  postProcessCollection_->GetEffectByName("CopyImage")},
		{ "ChromaticAberration", false, postProcessCollection_->GetEffectByName("ChromaticAberration")},
	};

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
//		ポストエフェクト処理（疑似
/////////////////////////////////////////////////////////////////////////////////////////
void EngineController::UpdatePostEffectControl(float dt) {
	postEffectGraph_ = system_->GetPostEffectGraph();

	// 2つのエフェクト取得
	auto chromaEffect = dynamic_cast<ChromaticAberrationEffect*>(
		system_->GetPostProcessCollection()->GetEffectByName("ChromaticAberration"));
	if (!chromaEffect) return;

	auto radialBlurEffect = dynamic_cast<RadialBlurEffect*>(
		system_->GetPostProcessCollection()->GetEffectByName("RadialBlur"));
	if (!radialBlurEffect) return;

	// LSHIFT で RadialBlur + ChromaticAberration を同時にON
	if (Input::GetInstance()->TriggerKey(DIK_LSHIFT)) {
		radialTimer_ = 0.0f;
		isRadialActive_ = true;

		for (auto& slot : postEffectSlots_) {
			slot.enabled = (slot.name == "RadialBlur" || slot.name == "ChromaticAberration");
		}

		postEffectGraph_->SetPassesFromList(postEffectSlots_);
	}

	// 演出中ならタイマー進行
	if (isRadialActive_) {
		radialTimer_ += dt;

		// フェードアウト: 1秒間で 1 → 0 に補間
		float t = 1.0f - (radialTimer_ / kRadialDurationSec_);
		t = std::clamp(t, 0.0f, 1.0f);

		chromaEffect->SetIntensity(0.2f * t);

		float blurWidth = 0.08f * t; // ブラー幅を0.08fから0.0fに補間
		radialBlurEffect->SetWidth(blurWidth);

		// 終了後はCopyImageのみに戻す
		if (radialTimer_ >= kRadialDurationSec_) {
			isRadialActive_ = false;

			for (auto& slot : postEffectSlots_) {
				slot.enabled = (slot.name == "CopyImage");
			}

			postEffectGraph_->SetPassesFromList(postEffectSlots_);
		}
	}
}

/////////////////////////////////////////////////////////////////////////////////////////
//		更新後処理
/////////////////////////////////////////////////////////////////////////////////////////
void EngineController::EndUpdate(){
	// UI描画
	engineUICore_->Render();	

	//UpdatePostEffectControl(ClockManager::GetInstance()->GetDeltaTime());
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

