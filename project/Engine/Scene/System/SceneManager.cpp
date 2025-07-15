#include "SceneManager.h"
/* ========================================================================
/* include space
/* ===================================================================== */
// scene
#include <Engine/Scene/Game/GameScene.h>
#include <Engine/Scene/System/SceneFactory.h>
#include <Engine/Scene/Test/TestScene.h>

// engine
#include <Engine/Application/Effects/Intermediary/FxIntermediary.h>
#include <Engine/Application/UI/Panels/HierarchyPanel.h>
#include <Engine/Application/UI/Panels/SceneSwitcherPanel.h>
#include <Engine/Graphics/Camera/Manager/CameraManager.h>
#include <Engine/Graphics/Context/GraphicsGroup.h>
#include <Engine/Graphics/Core/GraphicsSystem.h>
#include <Engine/Graphics/RenderTarget/Interface/IRenderTarget.h>
#include <Engine/Scene/Asset/SceneAsset.h>
#include <Engine/Scene/Controller/SceneControllerFactory.h>
#include <Engine/Scene/Serializer/SceneSerializer.h>
#include <Engine/Foundation/Clock/ClockManager.h>

/////////////////////////////////////////////////////////////////////////////////////////
//	コンストラクタ
/////////////////////////////////////////////////////////////////////////////////////////
SceneManager::SceneManager(DxCore* dxCore, GraphicsSystem* graphicsSystem)
	: pDxCore_(dxCore), pGraphicsSystem_(graphicsSystem){
	// ここでシーンをすべて生成しておく
	for (int i = 0; i < static_cast< int >(SceneType::count); ++i){
		scenes_[i] = SceneFactory::CreateScene(static_cast< SceneType >(i));
		scenes_[i]->SetTransitionRequestor(this);
	}


	currentSceneNo_ = static_cast< int >(SceneType::TEST);
#ifdef _DEBUG
	currentSceneNo_ = static_cast< int >(SceneType::TEST);
#endif // 

	nextSceneNo_ = currentSceneNo_;

}

SceneManager::~SceneManager(){}

/////////////////////////////////////////////////////////////////////////////////////////
//		初期化
/////////////////////////////////////////////////////////////////////////////////////////
void SceneManager::Initialize(){
#ifdef _DEBUG

	if (pEngineUI_){
		auto sceneSwitchPanel = std::make_unique<SceneSwitcherPanel>(this);

		sceneSwitchPanel->AddSceneOption("Game Scene", SceneType::PLAY);
		sceneSwitchPanel->AddSceneOption("Test Scene", SceneType::TEST);

		pEngineUI_->AddPanel(std::move(sceneSwitchPanel));
	}
#endif // _DEBUG

	FxIntermediary::GetInstance()->SetSceneContext(scenes_[currentSceneNo_]->GetSceneContext());
	scenes_[currentSceneNo_]->Initialize();

#ifdef _DEBUG
	auto* SceneObjectLibrary = scenes_[currentSceneNo_]->GetSceneContext()->GetObjectLibrary();
	pEngineUI_->GetHierarchyPanel()->SetSceneObjectLibrary(SceneObjectLibrary);
#endif // _DEBUG
}

/////////////////////////////////////////////////////////////////////////////////////////
//		更新
/////////////////////////////////////////////////////////////////////////////////////////
void SceneManager::Update(){
	/* ─ 新方式シーン ─ */
	if (usingAssetScene_){
		assetContext_->Update();
		if (controller_) controller_->OnUpdate(*assetContext_,
											   ClockManager::GetInstance()->GetDeltaTime());
		return;                               // 旧方式へは入らない
	}

	/* ─ 従来方式シーン ─ */
	if (currentSceneNo_ != nextSceneNo_){
		scenes_[currentSceneNo_]->CleanUp();
		currentSceneNo_ = nextSceneNo_;

		FxIntermediary::GetInstance()->SetSceneContext(
			scenes_[currentSceneNo_]->GetSceneContext());
		scenes_[currentSceneNo_]->Initialize();

	#ifdef _DEBUG
		if (pEngineUI_)
			pEngineUI_->NotifySceneContextChanged(
			scenes_[currentSceneNo_]->GetSceneContext());
	#endif
	}
	scenes_[currentSceneNo_]->Update();
}

/////////////////////////////////////////////////////////////////////////////////////////
//		描画
/////////////////////////////////////////////////////////////////////////////////////////
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

/////////////////////////////////////////////////////////////////////////////////////////
//		描画先のレンダーtarget
/////////////////////////////////////////////////////////////////////////////////////////
void SceneManager::DrawForRenderTarget(IRenderTarget* target){
	auto* cmd = pGraphicsSystem_->GetCommandList();
	target->SetRenderTarget(cmd);
	target->Clear(cmd);

	if (usingAssetScene_){
		assetContext_->Render(cmd, pGraphicsSystem_->GetPipelineService(),
							  target->GetRenderTargetType());
	} else{
		scenes_[currentSceneNo_]->Draw(cmd, pGraphicsSystem_->GetPipelineService(),
									   target->GetRenderTargetType());
	}
}

/////////////////////////////////////////////////////////////////////////////////////////
//		assetでシーンを変換
/////////////////////////////////////////////////////////////////////////////////////////
void SceneManager::ChangeSceneByAsset(const std::string& assetPath){
	/* ─ ① 旧 Asset シーンをアンロード ─ */
	if (usingAssetScene_){
		if (controller_) controller_->OnExit(*assetContext_);
		assetContext_.reset();
		controller_.reset();
		usingAssetScene_ = false;
	}

	/* ─ ② SceneAsset をロード ─ */
	SceneAsset asset;
	if (!JsonUtils::Load(assetPath, asset)){
		OutputDebugStringA(("SceneAsset load failed: " + assetPath + "\n").c_str());
		return;
	}

	/* ─ ③ SceneContext を生成 & レイアウト展開 ─ */
	assetContext_ = std::make_unique<SceneContext>();
	assetContext_->Initialize();
	SceneSerializer::Load(*assetContext_, asset.layout);

	/* ─ ④ Controller 生成 & OnEnter ─ */
	controller_ = SceneControllerFactory::Get().Create(asset.controller);
	if (controller_) controller_->OnEnter(*assetContext_, asset);

	/* ─ ⑤ UI / FxSystem などに通知 ─ */
	FxIntermediary::GetInstance()->SetSceneContext(assetContext_.get());
#ifdef _DEBUG
	if (pEngineUI_) pEngineUI_->NotifySceneContextChanged(assetContext_.get());
#endif

	usingAssetScene_ = true;
	currentAssetPath_ = assetPath;
}

/////////////////////////////////////////////////////////////////////////////////////////
//		エンジンuiを設定
/////////////////////////////////////////////////////////////////////////////////////////
void SceneManager::SetEngineUI([[maybe_unused]] EngineUICore* ui){
#ifdef _DEBUG
	pEngineUI_ = ui;
	auto* context = scenes_[currentSceneNo_]->GetSceneContext();
	pEngineUI_->NotifySceneContextChanged(context);
#endif // _DEBUG
}

/////////////////////////////////////////////////////////////////////////////////////////
//		シーンチェンジをリクエスト
/////////////////////////////////////////////////////////////////////////////////////////
void SceneManager::RequestSceneChange(SceneType nextScene){
	nextSceneNo_ = static_cast< int >(nextScene);
}

/////////////////////////////////////////////////////////////////////////////////////////
//		現在のシーンのコンテキストを取得
/////////////////////////////////////////////////////////////////////////////////////////
SceneContext* SceneManager::GetCurrentSceneContext() const{
	if (currentSceneNo_ >= 0 && currentSceneNo_ < static_cast< int >(scenes_.size())){
		return scenes_[currentSceneNo_]->GetSceneContext();
	}
	return nullptr;
}

