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
	LoadAssets();

	/* ----- Camera ----- */
	railCamera_ = std::make_unique<RailCamera>();
	railCamera_->Initialize();
	CameraManager::GetInstance()->SetType(CameraType::Type_Default);

	/* ----- Field ----- */
	modelField_ = SceneAPI::Instantiate<BaseGameObject>("terrain.obj", "field");
	modelField_->SetScale({300,300,300});
	modelField_->SetTranslate({-150,-150,0});
	modelField_->SetUvScale({10,10});
	modelField_->SetEnableRaycast(false);

	/* ----- Player ----- */
	player_ = SceneAPI::Instantiate<Player>("player.gltf", "player");
	player_->Initialize();
	// カメラ‐>プレイヤーの追従は Transform 参照を直接渡す方が安全
	player_->SetParent(&railCamera_->GetWorldTransform());

	/* ----- Bullet Container ----- */
	playerBulletContainer_ = SceneAPI::Instantiate<BulletContainer>("playerBulletContainer");
	player_->SetBulletContainer(playerBulletContainer_.get());

	/* ----- Enemy ----- */
	enemyCollection_ = SceneAPI::Instantiate<EnemyCollection>("enemyContainer");
	enemyCollection_->SetPlayerTransform(&player_->GetWorldTransform());
	enemyCollection_->CreateSpawners();

	player_->SetEnemyList(enemyCollection_->GetEnemies());
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

	/* その他 ============================*/
	sceneContext_->Update();
	CollisionManager::GetInstance()->UpdateCollisionAllCollider();

	if (Input::GetInstance()->TriggerKey(DIK_1)
		|| enemyCollection_->GetDeadEnemyCount() >= 10){//10タイ撃破
		if (transitionRequestor_){
			transitionRequestor_->RequestSceneChange(SceneType::TITLE);
		}
	}
}

void GameScene::Draw(ID3D12GraphicsCommandList* cmdList, PipelineService* psoService, RenderTargetType type){

	for (auto& playerSprite : player_->GetAllSprites()){
		spriteRenderer_->Register(playerSprite);
	}

	BaseScene::Draw(cmdList, psoService, type);
}


void GameScene::CleanUp(){
	// 3Dオブジェクトの描画を終了
	sceneContext_->GetObjectLibrary()->Clear();
	CollisionManager::GetInstance()->ClearColliders();
}

