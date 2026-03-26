#include "CalyxFrameWork.h"
/* ========================================================================
/*	include space
/* ===================================================================== */
// engine
#include <Engine/Application/Effects/FxSystem.h>
#include <Engine/Application/System/Environment.h>
#include <Engine/Application/UI/EngineUI/Core/EngineUICore.h>
#include <Engine/Foundation/Clock/ClockManager.h>
#include <Engine/Foundation/Input/Input.h>
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
		sceneManager_ = std::make_unique<CalyxScene::SceneManager>(system_->GetDxCore());
		sceneManager_->Initialize();

		/* PlaySession  (EditorCtx は SceneManager が作ったシーン 0 のものを使う) */
		playSession_ = std::make_unique<CalyxEditor::PlaySession>();
		playSession_->Initialize(sceneManager_->GetCurrentSceneContext());

		sceneManager_->BindPlaySession(playSession_.get());

		/* UI / Editor */
		engineUICore_ = std::make_unique<CalyxEditor::EngineUICore>();
		engineUICore_->Initialize();
		system_->SetEngineUICore(engineUICore_.get());

		if(auto* lvl = engineUICore_->GetLevelEditor()) {
			lvl->SetPlaySession(playSession_.get());
			lvl->SetSceneManager(sceneManager_.get());
		}

		editorCollection_ = std::make_unique<CalyxEditor::EditorCollection>();
		editorCollection_->InitializeEditors();

#if defined(_DEBUG) || defined(DEVELOP)
		engineUICore_->SetCameraForViewport(CameraManager::GetMain3d(), CameraManager::GetDebug());
#endif

		livePPService_.Initialize();
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
		livePPService_.Finalize();
		system_->Finalize();
		CoUninitialize();
	}

	////////////////////////////////////////////////////////////////////////////////
	bool CalyxFrameWork::Update() {
		float dt	   = ClockManager::GetInstance()->GetPlayerDeltaTime();
		float alwaysDt = ClockManager::GetInstance()->GetDeltaTime();

		BeginUpdate();

		playSession_->Update();

		sceneManager_->Update(dt, alwaysDt);

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

		livePPService_.Update();
	}

	////////////////////////////////////////////////////////////////////////////////
	void CalyxFrameWork::Render() {
		sceneManager_->Draw(graphicsSystem_->GetCommandList(), graphicsSystem_->GetPipelineService());

		system_->ExecutePostEffect(graphicsSystem_->GetPipelineService());

		sceneManager_->DrawNotAffectedFromPE(graphicsSystem_->GetCommandList(), graphicsSystem_->GetPipelineService());
		system_->EndFrame();
	}
} // namespace CalyxEngine