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

SceneManager::SceneManager(DxCore* dxCore)
	: pDxCore_(dxCore){
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

void SceneManager::Update(float dt){
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
	scenes_[currentSceneNo_]->Update(dt);

}

void SceneManager::PostUpdate(ID3D12GraphicsCommandList* cmdList,
							  PipelineService* pipelineService){
	scenes_[currentSceneNo_]->PostUpdate(cmdList,pipelineService);
}

void SceneManager::DrawNotAffectedFromPE(ID3D12GraphicsCommandList* cmdList, PipelineService* pipelineService){
	auto* backBuffer = pDxCore_->GetRenderTargetCollection().Get("BackBuffer");

	backBuffer->SetRenderTarget(cmdList);

	// スプライト描画
	if (auto* baseScene = scenes_[currentSceneNo_].get()){
		baseScene->DrawSpritesOnly(cmdList,pipelineService);
	}
}

void SceneManager::Draw(ID3D12GraphicsCommandList* cmdList, PipelineService* psoService){
	CameraManager::GetInstance()->SetType(Type_Default);
	auto* gameRT = pDxCore_->GetRenderTargetCollection().Get("Offscreen");
	DrawForRenderTarget(gameRT, cmdList,psoService);

#ifdef _DEBUG
	CameraManager::GetInstance()->SetType(Type_Debug);
	auto* debugRT = pDxCore_->GetRenderTargetCollection().Get("DebugView");
	DrawForRenderTarget(debugRT, cmdList, psoService);
#endif // _DEBUG

	//プリミティブ描画
	GraphicsGroup::GetInstance()->SetCommand(cmdList, PipelineType::Line, BlendMode::NORMAL);
	CameraManager::SetCommand(cmdList, PipelineType::Line);
	PrimitiveDrawer::GetInstance()->Render();
	PrimitiveDrawer::GetInstance()->ClearMesh();
}

void SceneManager::DrawForRenderTarget(IRenderTarget* target, 
									   ID3D12GraphicsCommandList* cmdList,
									   PipelineService*psoService){
	// 出力先RT設定
	target->SetRenderTarget(cmdList);
	target->Clear(cmdList);

	scenes_[currentSceneNo_]->Draw(cmdList, psoService, target->GetRenderTargetType());
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

