#include "TitleScene.h"
/////////////////////////////////////////////////////////////////////////////////////////
//	include
/////////////////////////////////////////////////////////////////////////////////////////

// scene
#include <Engine/Scene/System/SceneManager.h>

// engine
#include <Engine/Application/Input/Input.h>
#include <Engine/Application/System/Enviroment.h>
#include <Engine/Assets/Texture/TextureManager.h>
#include <Engine/Collision/CollisionManager.h>
#include <Engine/Graphics/Camera/Manager/CameraManager.h>
#include <Engine/Graphics/Context/GraphicsGroup.h>
#include <Engine/Objects/3D/Actor/SceneObjectManager.h>
#include <Engine/Scene/Serializer/SceneSerializer.h>
#include <Engine/Scene/Utility/SceneUtility.h>

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
		transitionRequestor_->RequestSceneChange(SceneType::PLAY);
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

	for(auto& sprite : menu_->GetAllButtonImage()) {
		spriteRenderer_->Register(sprite);
	}
}