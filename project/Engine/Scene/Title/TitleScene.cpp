#include "TitleScene.h"
/////////////////////////////////////////////////////////////////////////////////////////
//	include
/////////////////////////////////////////////////////////////////////////////////////////

// scene
#include <Engine/Scene/System/SceneManager.h>

// engine
#include <Engine/Collision/CollisionManager.h>
#include <Engine/Scene/Serializer/SceneSerializer.h>
#include <Engine/Scene/Utility/SceneUtility.h>
#include <Game/Scene/Utility/SceneTypeUtil.h>

/////////////////////////////////////////////////////////////////////////////////////////
//	コンストラクタ/デストラクタ
/////////////////////////////////////////////////////////////////////////////////////////
TitleScene::TitleScene() {
	// シーン名を設定
	// IScene::SetSceneName("TitleScene");
	SetSceneName("TitleScene");
}

/////////////////////////////////////////////////////////////////////////////////////////
//	アセットのロード
/////////////////////////////////////////////////////////////////////////////////////////
void TitleScene::LoadAssets() {}

/////////////////////////////////////////////////////////////////////////////////////////
//	初期化処理
/////////////////////////////////////////////////////////////////////////////////////////
void TitleScene::Initialize() {
	sceneContext_->Initialize(false);

	// シーンデータ読み込み
	SceneSerializer::Load(*sceneContext_, "Resources/Assets/Scenes/TitleScene.scene");

	LoadAssets();

	//=========================
	// ボタンで発行される関数
	//=========================
	transitionForGameScene_ = [this] {
		transitionRequestor_->RequestSceneChange(GameSceneUtil::ToSceneId(SceneType::PLAY));
	};

	endGameReqest_ = [this] {
		GameEndReqest();
	};

	//=========================
	// menuボタン
	//=========================
	menu_ = std::make_unique<TitleMenuController>();
	menu_->SetMenuEvent(transitionForGameScene_);
	menu_->SetGameEndEvent(endGameReqest_);
}

void TitleScene::Update([[maybe_unused]] float dt) {
	/* 3dObject ============================*/
	/* その他 ============================*/
	menu_->Update(dt);


	CollisionManager::GetInstance()->UpdateCollisionAllCollider();
}

void TitleScene::CleanUp() {
	// 3Dオブジェクトの描画を終了
	sceneContext_->GetObjectLibrary()->Clear();
	CollisionManager::GetInstance()->ClearColliders();
}

void TitleScene::Draw(ID3D12GraphicsCommandList* cmdList, PipelineService* psoService, RenderTargetType type) {
	BaseScene::Draw(cmdList, psoService, type);

	// スプライトの描画
	for(auto& sprite : menu_->GetAllButtonImage()) {
		spriteRenderer_->Register(sprite);
	}
}