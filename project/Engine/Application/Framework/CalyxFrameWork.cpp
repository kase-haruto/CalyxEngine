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

#include <CalyxEngine/Application.h>
#include <CalyxEngine/SceneRegistry.h>

namespace CalyxEngine {
	////////////////////////////////////////////////////////////////////////////////
	//  engine 初期化
	////////////////////////////////////////////////////////////////////////////////
	void CalyxFrameWork::Initialize(HINSTANCE hInstance, Calyx::Application* application) {
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
		if(application) {
			Calyx::SceneRegistry registry(*sceneManager_);
			application->RegisterScenes(registry);
		}

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
			lvl->SetSceneManager(sceneManager_.get());
		}

		editorCollection_ = std::make_unique<EditorCollection>();
		editorCollection_->InitializeEditors();

#if defined(_DEBUG) || defined(DEVELOP)
		engineUICore_->SetCameraForViewport(CameraManager::GetMain3d(), CameraManager::GetDebug());
#endif
	}

	////////////////////////////////////////////////////////////////////////////////
	//  メインループ
	////////////////////////////////////////////////////////////////////////////////
	void CalyxFrameWork::Run(Calyx::Application* application) {
		while(!system_->ProcessMessage()) {
			if(!Update(application)) break;
			Render(application);
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
	bool CalyxFrameWork::Update(Calyx::Application* application) {
		float dt	   = ClockManager::GetInstance()->GetPlayerDeltaTime();
		float alwaysDt = ClockManager::GetInstance()->GetDeltaTime();

		BeginUpdate();

		playSession_->Update();
		if(application) {
			application->OnUpdate();
		}

		sceneManager_->Update(dt, alwaysDt);
		engineUICore_->SetEditorUiEnabled(application ? application->ShouldRenderEngineUi() : true);

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
	}

	////////////////////////////////////////////////////////////////////////////////
	void CalyxFrameWork::Render(Calyx::Application* application) {
		sceneManager_->Draw(graphicsSystem_->GetCommandList(), graphicsSystem_->GetPipelineService());
		if(application) {
			application->OnRender();
		}

		system_->ExecutePostEffect(graphicsSystem_->GetPipelineService());

		sceneManager_->DrawNotAffectedFromPE(graphicsSystem_->GetCommandList(), graphicsSystem_->GetPipelineService());
		system_->EndFrame();
	}
} // namespace CalyxEngine
