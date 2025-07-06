#include "GameScene.h"
/////////////////////////////////////////////////////////////////////////////////////////
//	include
/////////////////////////////////////////////////////////////////////////////////////////

// scene
#include <Engine/Scene/System/SceneManager.h>
#include <Engine/Scene/Utirity/SceneUtility.h>

// engine
#include <Engine/Graphics/Context/GraphicsGroup.h>
#include <Engine/Application/Input/Input.h>
#include <Engine/Graphics/Camera/Manager/CameraManager.h>
#include <Engine/Objects/3D/Actor/SceneObjectManager.h>
#include <Engine/Collision/CollisionManager.h>
#include <Engine/Graphics/Pipeline/Service/PipelineService.h>

/////////////////////////////////////////////////////////////////////////////////////////
//	コンストラクタ/デストラクタ
/////////////////////////////////////////////////////////////////////////////////////////
GameScene::GameScene() {
	// シーン名を設定
	//IScene::SetSceneName("GameScene");
	SetSceneName("GameScene");

}

/////////////////////////////////////////////////////////////////////////////////////////
//	アセットのロード
/////////////////////////////////////////////////////////////////////////////////////////
void GameScene::LoadAssets() {
}


/////////////////////////////////////////////////////////////////////////////////////////
//	初期化処理
/////////////////////////////////////////////////////////////////////////////////////////
void GameScene::Initialize(){
	sceneContext_->Initialize();
	LoadAssets();

	CameraManager::GetInstance()->SetType(CameraType::Type_Default);
	
	//=========================
	// グラフィック関連
	//=========================
	railCamera_ = std::make_unique<RailCamera>();
	railCamera_->Initialize();

	modelField_ = sceneContext_->GetObjectLibrary()->CreateAndAddObject<BaseGameObject>("terrain.obj", "field");
	modelField_->SetScale({300.0f,300.0f,300.0f});
	modelField_->SetTranslate({ 0.0f, -50.0f, 0.0f });

	modelFieldBack_ = sceneContext_->GetObjectLibrary()->CreateAndAddObject<BaseGameObject>("terrain.obj", "field_back");
	modelFieldBack_->SetScale({ 300.0f, 300.0f, 300.0f });
	modelFieldBack_->SetTranslate({ 0.0f, -50.0f, 1000.0f });

	//player
	player_ = sceneContext_->GetObjectLibrary()->CreateAndAddObject<Player>("player.gltf", "player");
	player_->Initialize();
	player_->SetParent(&railCamera_->GetWorldTransform());

	playerBulletContainer_ = sceneContext_->AddEditorObject(std::make_unique<BulletContainer>("playerBulletContainer"));
	playerBulletContainer_->SetSceneContext(sceneContext_.get());
	player_->SetBulletContainer(playerBulletContainer_);

	enemyCollection_ = sceneContext_->AddEditorObject(std::make_unique<EnemyCollection>("enemyContainer"));
	enemyCollection_->SetSceneContext(sceneContext_.get());

	enemyCollection_->SetPlayerTransform(&player_->GetWorldTransform());

	enemyCollection_->CreateSpawners();


	//===================================================================*/
	//                    editor
	//===================================================================*/
}

void GameScene::Update(){
	/* カメラ関連更新 ============================*/
	railCamera_->Update();
	CameraManager::GetCamera3d()->SetCamera(railCamera_->GetPosition(), railCamera_->GetRotation());
	CameraManager::Update();

	skyBox_->Update();
	
	/* 3dObject ============================*/
	//地面の更新
	modelField_->Update();
	//プレイヤーの更新
	player_->Update();

	/* その他 ============================*/
	sceneContext_->Update();
	CollisionManager::GetInstance()->UpdateCollisionAllCollider();

	if (Input::GetInstance()->TriggerKey(DIK_1)
		||enemyCollection_->GetDeadEnemyCount()>=10) {//10タイ撃破
		if (transitionRequestor_) {
			transitionRequestor_->RequestSceneChange(SceneType::TITLE);
		}
	}
}

void GameScene::Draw(ID3D12GraphicsCommandList* cmdList, PipelineService* psoService){

	for (auto& playerSprite:player_->GetAllSprites()){
		spriteRenderer_->Register(playerSprite);
	}

	BaseScene::Draw(cmdList, psoService);
}


void GameScene::CleanUp(){
	// 3Dオブジェクトの描画を終了
	sceneContext_->GetObjectLibrary()->Clear();
	CollisionManager::GetInstance()->ClearColliders();
}

