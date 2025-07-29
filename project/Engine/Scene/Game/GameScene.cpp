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

	/* ----- Camera ----- */
	railCamera_ = SceneAPI::Instantiate<RailCamera>("railCamera");
	railCamera_->Initialize();
	CameraManager::GetMain3d()->SetParent(railCamera_);

	/* ----- Field ----- */
	modelField_ = SceneAPI::Instantiate<BaseGameObject>("player.gltf", "field");
	modelField_->SetScale({300,300,300});
	modelField_->SetTranslate({-150,-150,0});
	modelField_->SetUvScale({10,10});
	modelField_->SetEnableRaycast(false);

	/* ----- Player ----- */
	PlayerInstaller playerInstaller;
	player_ = playerInstaller.InstallPlayer();
	// レールカメラをプレイヤーの親に設定
	player_->SetParent(&railCamera_->GetWorldTransform());
	player_->Initialize();

	/* ----- Enemy ----- */
	enemyCollection_ = SceneAPI::Instantiate<EnemyCollection>("enemyContainer");
	enemyCollection_->SetPlayerTransform(&player_->GetWorldTransform());
	enemyCollection_->CreateSpawners();

}

void GameScene::Update(float dt){
	ImGui::Begin("Play Control");

	switch (playSession_.GetMode()){
		case EngineMode::Editor:
			if (ImGui::Button("▶ Play")) playSession_.Enter();
			break;

		case EngineMode::Playing:
			if (ImGui::Button("■ Stop")) playSession_.Exit();
			ImGui::SameLine();
			if (ImGui::Button("⏸ Pause")) playSession_.TogglePause();
			ImGui::SameLine();
			if (ImGui::Button("🔁 Restart")) playSession_.Restart();
			break;

		case EngineMode::Paused:
			if (ImGui::Button("■ Stop")) playSession_.Exit();
			ImGui::SameLine();
			if (ImGui::Button("▶ Resume")) playSession_.TogglePause();
			ImGui::SameLine();
			if (ImGui::Button("⏭ Step")) playSession_.StepOnce();
			ImGui::SameLine();
			if (ImGui::Button("🔁 Restart")) playSession_.Restart();
			break;
	}

	ImGui::End();
	playSession_.Update(dt);

	/* カメラ関連更新 ============================*/
	player_->SetEnemyList(enemyCollection_->GetEnemies());

	/* その他 ============================*/
	CollisionManager::GetInstance()->UpdateCollisionAllCollider();
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

