#include "GameScene.h"
/////////////////////////////////////////////////////////////////////////////////////////
//	include
/////////////////////////////////////////////////////////////////////////////////////////

// scene
#include <Engine/Scene/System/SceneManager.h>

// engine
#include <Engine/Graphics/Context/GraphicsGroup.h>
#include <Engine/Application/Input/Input.h>
#include <Engine/Graphics/Camera/Manager/CameraManager.h>
#include <Engine/Objects/3D/Actor/SceneObjectManager.h>
#include <Engine/Collision/CollisionManager.h>
#include <Engine/Graphics/Pipeline/Service/PipelineService.h>
#include <Engine/Scene/Utility/SceneUtility.h>
#include <Engine/Scene/Serializer/SceneSerializer.h>
// game
#include <Game/3dObject/Actor/Bullet/Register/BulletRegistrar.h>
#include <Game/Installer/Player/PlayerInstaller.h>

/////////////////////////////////////////////////////////////////////////////////////////
//	コンストラクタ/デストラクタ
/////////////////////////////////////////////////////////////////////////////////////////
GameScene::GameScene(){
	// シーン名を設定
	//IScene::SetSceneName("GameScene");
	SetSceneName("GameScene");

}

/////////////////////////////////////////////////////////////////////////////////////////
//	アセットのロード
/////////////////////////////////////////////////////////////////////////////////////////
void GameScene::LoadAssets(){}


/////////////////////////////////////////////////////////////////////////////////////////
//	初期化処理
/////////////////////////////////////////////////////////////////////////////////////////
void GameScene::Initialize(){
	sceneContext_->Initialize();

	SceneSerializer::Load(*sceneContext_, "Resources/Assets/Scenes/GameScene.scene");

	BaseScene::Initialize();

	LoadAssets();
	//弾の登録
	BulletRegistrar::RegisterAll();

	auto ctx = SceneContext::Current();
	auto ground = ctx->FindObjectByName<BaseGameObject>("field");
	ground->SetEnableRaycast(false);

	attackSprite_ = std::make_unique<Sprite>("Textures/attackUI.png");
	Vector2 attackUiPos = Vector2(1280.0f * 0.5f, 720.0f - 100.0f);
	Vector2 attackUiSize = Vector2(128.0f, 128.0f);
	attackSprite_->Initialize(attackUiPos, attackUiSize);
	attackSprite_->SetAnchorPoint(Vector2(0.5f, 0.5f));

}

void GameScene::Update([[maybe_unused]]float dt){
	auto ctx = SceneContext::Current();
	auto player = ctx->FindFirst<Player>();
	auto enemyCol = ctx->FindFirst<EnemyCollection>();
	if (player && enemyCol) {
		player->SetEnemyList(enemyCol->GetEnemies());
	}

	attackSprite_->Update();


	/* その他 ============================*/
	CollisionManager::GetInstance()->UpdateCollisionAllCollider();
}

void GameScene::Draw(ID3D12GraphicsCommandList* cmdList, PipelineService* psoService, RenderTargetType type){
	auto ctx = SceneContext::Current();
	auto player = ctx->FindFirst<Player>();
	for (auto& playerSprite : player->GetAllSprites()){
		spriteRenderer_->Register(playerSprite);
	}

	spriteRenderer_->Register(attackSprite_.get());

	BaseScene::Draw(cmdList, psoService, type);
}


void GameScene::CleanUp(){
	auto ctx = SceneContext::Current();
	// 3Dオブジェクトの描画を終了
	ctx->GetObjectLibrary()->Clear();
	CollisionManager::GetInstance()->ClearColliders();
}

