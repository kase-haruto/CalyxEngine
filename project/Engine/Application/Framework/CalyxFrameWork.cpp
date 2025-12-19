#include "CalyxFrameWork.h"
/* ========================================================================
/*	include space
/* ===================================================================== */
// engine
#include <Engine/Application/Effects/FxSystem.h>
#include <Engine/Foundation/Input/Input.h>
#include <Engine/Application/System/Enviroment.h>
#include <Engine/Application/UI/EngineUI/Core/EngineUICore.h>
#include <Engine/Foundation/Clock/ClockManager.h>
#include <Engine/Scene/System/SceneManager.h>
namespace CalyxEngine {
	////////////////////////////////////////////////////////////////////////////////
	//  engine 初期化
	////////////////////////////////////////////////////////////////////////////////
	void CalyxFrameWork::Initialize(HINSTANCE hInstance) {
		/* COM */
		CoInitializeEx(0, COINIT_MULTITHREADED);

		/* System & Graphics */
		system_ = std::make_unique<CalyxCore>();
		system_->Initialize(hInstance, kWindowWidth, kWindowHeight, windowTitle);

		graphicsSystem_ = std::make_unique<GraphicsSystem>();
		graphicsSystem_->Initialize();
		system_->InitializePostProcess(graphicsSystem_->GetPipelineService());

		/* SceneManager */
		sceneManager_ = std::make_unique<SceneManager>(system_->GetDxCore());
		sceneManager_->Initialize();

		/* PlaySession  (EditorCtx は SceneManager が作ったシーン 0 のものを使う) */
		playSession_ = std::make_unique<PlaySession>();
		playSession_->Initialize(sceneManager_->GetCurrentSceneContext());

		sceneManager_->BindPlaySession(playSession_.get());

		/* UI / Editor */
		engineUICore_ = std::make_unique<EngineUICore>();
		engineUICore_->Initialize();
		system_->SetEngineUICore(engineUICore_.get());

		if(auto* lvl = engineUICore_->GetLevelEditor()) {
			lvl->SetPlaySession(playSession_.get());
		}

		editorCollection_ = std::make_unique<EditorCollection>();
		editorCollection_->InitializeEditors();

		// auto* ppEditor = dynamic_cast< PostProcessEditor* >(editorCollection_->GetEditor(EditorCollection::EditorType::PostProcess));

		// ppEditor->SetPostEffectCollection(system_->GetPostProcessCollection());

#if defined(_DEBUG) || defined(DEVELOP)
		engineUICore_->SetCameraForViewport(CameraManager::GetMain3d(), CameraManager::GetDebug());
#endif
	}

	////////////////////////////////////////////////////////////////////////////////
	//  メインループ
	////////////////////////////////////////////////////////////////////////////////
	void CalyxFrameWork::Run() {
		while(!system_->ProcessMessage()) {
			if(!Update()) break;
			Render();
			if(CalyxFoundation::Input::TriggerKey(DIK_ESCAPE) ||
			   sceneManager_->GetIsEndGame()) break;
		}
	}

	////////////////////////////////////////////////////////////////////////////////
	void CalyxFrameWork::Finalize() {
		system_->Finalize();
		CoUninitialize();
	}

	////////////////////////////////////////////////////////////////////////////////
	bool CalyxFrameWork::Update() {
		float dt = ClockManager::GetInstance()->GetDeltaTime();

		BeginUpdate();

		playSession_->Update();

		sceneManager_->Update(dt);

		EndUpdate();
		return true;
	}

	////////////////////////////////////////////////////////////////////////////////
	void CalyxFrameWork::BeginUpdate() {
		system_->BeginFrame();
		engineUICore_->Update();
	}

	////////////////////////////////////////////////////////////////////////////////
	void CalyxFrameWork::EndUpdate() {
		sceneManager_->PostUpdate(graphicsSystem_->GetCommandList(), graphicsSystem_->GetPipelineService());

		engineUICore_->Render();

		// uto* ppEditor = dynamic_cast< PostProcessEditor* >(editorCollection_->GetEditor(EditorCollection::EditorType::PostProcess));
		// ppEditor->ApplyToGraph(system_->GetPostEffectGraph());
	}

	////////////////////////////////////////////////////////////////////////////////
	void CalyxFrameWork::Render() {
		sceneManager_->Draw(graphicsSystem_->GetCommandList(), graphicsSystem_->GetPipelineService());

		system_->ExecutePostEffect(graphicsSystem_->GetPipelineService());

		sceneManager_->DrawNotAffectedFromPE(graphicsSystem_->GetCommandList(), graphicsSystem_->GetPipelineService());
		system_->EndFrame();
	}
} // namespace CalyxEngine