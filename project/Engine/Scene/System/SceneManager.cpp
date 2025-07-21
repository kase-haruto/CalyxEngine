#include "SceneManager.h"

// scene
#include <Engine/Scene/Game/GameScene.h>
#include <Engine/Scene/System/SceneFactory.h>
#include <Engine/Scene/Test/TestScene.h>

// engine
#include <Engine/Application/UI/Panels/HierarchyPanel.h>
#include <Engine/Application/UI/Panels/SceneSwitcherPanel.h>
#include <Engine/Graphics/Camera/Manager/CameraManager.h>
#include <Engine/Graphics/RenderTarget/Interface/IRenderTarget.h>
#include <Engine/Graphics/Context/GraphicsGroup.h>
#include <Engine/Objects/3D/Actor/SceneObjectManager.h>
#include <Engine/Application/Input/Input.h>
#include <Engine/Graphics/Core/GraphicsSystem.h>

SceneManager::SceneManager(DxCore* dxCore, GraphicsSystem* graphicsSystem)
	: pDxCore_(dxCore), pGraphicsSystem_(graphicsSystem){
	// ここでシーンをすべて生成しておく
	for (int i = 0; i < static_cast< int >(SceneType::count); ++i){
		scenes_[i] = SceneFactory::CreateScene(static_cast< SceneType >(i));
		scenes_[i]->SetTransitionRequestor(this);
	}


	currentSceneNo_ = static_cast< int >(SceneType::PLAY);
#ifdef _DEBUG
	currentSceneNo_ = static_cast< int >(SceneType::PLAY);
#endif // 

	nextSceneNo_ = currentSceneNo_;

}

SceneManager::~SceneManager(){}

void SceneManager::Initialize(){
#ifdef _DEBUG

	if (pEngineUI_){
		auto sceneSwitchPanel = std::make_unique<SceneSwitcherPanel>(this);

		sceneSwitchPanel->AddSceneOption("Game Scene", SceneType::PLAY);
		sceneSwitchPanel->AddSceneOption("Test Scene", SceneType::TEST);

		pEngineUI_->AddPanel(std::move(sceneSwitchPanel));
	}
#endif // _DEBUG

	scenes_[currentSceneNo_]->GetSceneContext()->MakeCurrent();
	scenes_[currentSceneNo_]->Initialize();

#ifdef _DEBUG
	auto* SceneObjectLibrary = scenes_[currentSceneNo_]->GetSceneContext()->GetObjectLibrary();
	pEngineUI_->GetHierarchyPanel()->SetSceneObjectLibrary(SceneObjectLibrary);
#endif // _DEBUG
}

void SceneManager::Update(){
	if (currentSceneNo_ != nextSceneNo_){
		// いったん現在シーンをクリーンアップ
		scenes_[currentSceneNo_]->CleanUp();


		// シーン番号を更新
		currentSceneNo_ = nextSceneNo_;

		// 新しい SceneContext を取得
		auto* newCtx = scenes_[currentSceneNo_]->GetSceneContext();

		//ctx を更新
		newCtx->MakeCurrent();

		scenes_[currentSceneNo_]->Initialize();

	}

	// 現在のシーンを更新
	scenes_[currentSceneNo_]->Update();
}

void SceneManager::Draw(){
	CameraManager::GetInstance()->SetType(Type_Default);
	auto* gameRT = pDxCore_->GetRenderTargetCollection().Get("Offscreen");
	DrawForRenderTarget(gameRT);

#ifdef _DEBUG
	CameraManager::GetInstance()->SetType(Type_Debug);
	auto* debugRT = pDxCore_->GetRenderTargetCollection().Get("DebugView");
	DrawForRenderTarget(debugRT);
#endif // _DEBUG

	//プリミティブ描画
	auto* cmd = pGraphicsSystem_->GetCommandList();
	GraphicsGroup::GetInstance()->SetCommand(cmd, PipelineType::Line, BlendMode::NORMAL);
	CameraManager::SetCommand(cmd, PipelineType::Line);
	PrimitiveDrawer::GetInstance()->Render();
	PrimitiveDrawer::GetInstance()->ClearMesh();
}

void SceneManager::DrawForRenderTarget(IRenderTarget* target){
	auto* cmd = pGraphicsSystem_->GetCommandList();

	// 出力先RT設定
	target->SetRenderTarget(cmd);
	target->Clear(cmd);

	scenes_[currentSceneNo_]->Draw(cmd, pGraphicsSystem_->GetPipelineService(), target->GetRenderTargetType());
}

void SceneManager::SetEngineUI([[maybe_unused]] EngineUICore* ui){
#ifdef _DEBUG
	pEngineUI_ = ui;
#endif // _DEBUG
}

void SceneManager::RequestSceneChange(SceneType nextScene){
	nextSceneNo_ = static_cast< int >(nextScene);
}

SceneContext* SceneManager::GetCurrentSceneContext() const{
	if (currentSceneNo_ >= 0 && currentSceneNo_ < static_cast< int >(scenes_.size())){
		return scenes_[currentSceneNo_]->GetSceneContext();
	}
	return nullptr;
}

