

/////////////////////////////////////////////////////////////////////////////////////////
//	include
/////////////////////////////////////////////////////////////////////////////////////////

// scene
#include "Game/Scene/Details/SceneType.h"
#include "Game/Scene/Utility/SceneTypeUtil.h"
#include <Game/DemoPlayer/DemoPlayer.h>
#include <Game/Scene/Gameover/GameoverScene.h>

// engine
#include <Engine/Collision/CollisionManager.h>
#include <Engine/Foundation/Input/Input.h>
#include <Engine/Foundation/Utility/Func/MyFunc.h>
#include <Engine/Scene/Context/SceneContext.h>
#include <Engine/Scene/Serializer/SceneSerializer.h>
// lib

/////////////////////////////////////////////////////////////////////////////////////////
//	コンストラクタ/デストラクタ
/////////////////////////////////////////////////////////////////////////////////////////
GameoverScene::GameoverScene() {
	// シーン名を設定
	GameoverScene::SetSceneName("GameoverScene");
}

/////////////////////////////////////////////////////////////////////////////////////////
//	アセットのロード
/////////////////////////////////////////////////////////////////////////////////////////
void GameoverScene::LoadAssets() {}

/////////////////////////////////////////////////////////////////////////////////////////
//	初期化処理
/////////////////////////////////////////////////////////////////////////////////////////
void GameoverScene::Initialize() {
	sceneContext_->Initialize();

	sceneContext_->SetSceneName("GameoverScene");

	BaseScene::Initialize();

	SceneSerializer::Load(*sceneContext_, "Resources/Assets/Scenes/Gameover.scene");

	LoadAssets();

	//=========================
	// グラフィック関連
	//=========================
	pauseBg_ = std::make_unique<Sprite>("Textures/uvChecker.dds");
	pauseBg_->Initialize({0.0f, 0.0f}, {1280.0f, 720.0f});
	pauseBg_->SetColor({1.0f, 0.0f, 0.0f, 1.0f});
	pauseBg_->Update();
}

/////////////////////////////////////////////////////////////////////////////////////////
//	更新処理
/////////////////////////////////////////////////////////////////////////////////////////
void GameoverScene::Update([[maybe_unused]] float dt) {

	if(CalyxFoundation::Input::TriggerKey(DIK_LCONTROL) && CalyxFoundation::Input::TriggerKey(DIK_7)) {
		transitionRequestor_->RequestSceneChange(GameSceneUtil::ToSceneId(SceneType::TEST));
	}

	// 衝突判定
	CollisionManager::GetInstance()->UpdateCollisionAllCollider();
}

void GameoverScene::Draw(ID3D12GraphicsCommandList* cmdList, PipelineService* psoService, IRenderTarget* rt) {

	//========================================================//
	//	spriteの登録
	//========================================================//
	spriteRenderer_->Register(pauseBg_.get());

	// シーン上のオブジェクトの描画
	BaseScene::Draw(cmdList, psoService, rt);
}

void GameoverScene::CleanUp() {
	// 3Dオブジェクトの描画を終了
	sceneContext_->GetObjectLibrary()->Clear();
	CollisionManager::GetInstance()->ClearColliders();
}