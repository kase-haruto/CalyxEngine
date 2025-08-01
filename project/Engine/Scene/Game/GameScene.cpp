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

	BaseScene::Initialize();

	LoadAssets();

	//弾の登録
	BulletRegistrar::RegisterAll();

	///* ----- Camera ----- */
	//railCamera_ = SceneAPI::Instantiate<RailCamera>("railCamera");
	//railCamera_->Initialize();

	///* ----- Field ----- */
	//modelField_ = SceneAPI::Instantiate<BaseGameObject>("terrain.obj", "field");
	//modelField_->SetScale({300,300,300});
	//modelField_->SetTranslate({-150,-150,0});
	//modelField_->SetUvScale({10,10});
	//modelField_->SetEnableRaycast(false);

	///* ----- Player ----- */
	//player_ = SceneAPI::Instantiate<Player>("player.gltf", "player");
	////player_->SetParent(&railCamera_->GetWorldTransform());
	//player_->Initialize();

	///* ----- Enemy ----- */
	//enemyCollection_ = SceneAPI::Instantiate<EnemyCollection>("enemyContainer");
	//enemyCollection_->SetPlayerTransform(&player_->GetWorldTransform());
	//enemyCollection_->CreateSpawners();

	emitter_ = std::make_unique<GpuFxEmitter>();
	emitter_->Initialize();

	sceneContext_->GetFxSystem()->AddEmitter(emitter_);
}

void GameScene::Update([[maybe_unused]]float dt){

	/* カメラ関連更新 ============================*/

	//player_->SetEnemyList(enemyCollection_->GetEnemies());
	emitter_->Update(dt);

	/* その他 ============================*/
	CollisionManager::GetInstance()->UpdateCollisionAllCollider();
}

void GameScene::Draw(ID3D12GraphicsCommandList* cmdList, PipelineService* psoService, RenderTargetType type){

	//for (auto& playerSprite : player_->GetAllSprites()){
	//	spriteRenderer_->Register(playerSprite);
	//}

	BaseScene::Draw(cmdList, psoService, type);
}


void GameScene::CleanUp(){
	// 3Dオブジェクトの描画を終了
	sceneContext_->GetObjectLibrary()->Clear();
	CollisionManager::GetInstance()->ClearColliders();
}

