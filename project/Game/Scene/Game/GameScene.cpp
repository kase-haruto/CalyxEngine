#include "GameScene.h"

/////////////////////////////////////////////////////////////////////////////////////////
//  include
/////////////////////////////////////////////////////////////////////////////////////////

// scene
#include <Engine/Scene/System/SceneManager.h>

// engine
#include <Engine/Application/Input/Input.h>
#include <Engine/Collision/CollisionManager.h>
#include <Engine/Graphics/Camera/Action/CameraTurnAroundAction.h>
#include <Engine/Objects/2D/NumbersSprite/NumbersSprite.h>
#include <Engine/Objects/3D/Actor/SceneObjectManager.h>
#include <Engine/Scene/Serializer/SceneSerializer.h>
#include <Engine/Scene/Utility/SceneUtility.h>

// game
#include <Game/3dObject/Actor/Bullet/Register/BulletRegistrar.h>
#include <Game/Battle/Shooting/Score/ScoreService.h>
#include <Game/Installer/Enemy/EnemyEngagementInstaller.h>
#include <Game/Installer/Player/PlayerInstaller.h>

/////////////////////////////////////////////////////////////////////////////////////////
//  ctor / dtor
/////////////////////////////////////////////////////////////////////////////////////////
GameScene::GameScene() { SetSceneName("Game"); }

GameScene::~GameScene() = default;

/////////////////////////////////////////////////////////////////////////////////////////
//  アセットのロード
/////////////////////////////////////////////////////////////////////////////////////////
void GameScene::LoadAssets() {}

/////////////////////////////////////////////////////////////////////////////////////////
//  初期化処理
/////////////////////////////////////////////////////////////////////////////////////////
void GameScene::Initialize() {
	// SceneContext 初期化
	sceneContext_->Initialize();

	// シーンデータ読み込み
	SceneSerializer::Load(*sceneContext_,"Resources/Assets/Scenes/GameScene.scene");

	// ベース初期化
	BaseScene::Initialize();

	// アセット読み込み
	LoadAssets();

	// 弾の登録
	BulletRegistrar::RegisterAll();

	if(auto* ctx = SceneContext::Current()) { if(auto ground = ctx->FindObjectByName<BaseGameObject>("field")) { ground->SetEnableRaycast(false); } }

	// UI
	attackSprite_ = std::make_unique<Sprite>("Textures/attackUI.png");
	{
		Vector2 attackUiPos  = Vector2(1280.0f * 0.5f,720.0f - 100.0f);
		Vector2 attackUiSize = Vector2(128.0f,128.0f);
		attackSprite_->Initialize(attackUiPos,attackUiSize);
		attackSprite_->SetAnchorPoint(Vector2(0.5f,0.5f));
	}

	// プレイヤー基本セットアップ
	{
		auto            player = sceneContext_->FindFirst<Player>();
		PlayerInstaller installer;
		installer.InstallPlayer(player);
		wPlayer_ = player; // Draw でスプライト拾うために持っておく
	}

	// 敵セットアップ

	{
		//敵弾コンテナ
		enemyBulletContainer_ = std::make_unique<EnemyBulletContainer>("EnemyBulletContainer");

		enemyBinding_ = std::make_unique<EnemyRuntimeBindingService>();
		enemyBinding_->OnSceneLoaded(*sceneContext_,enemyBulletContainer_.get());

		occurrenceBoss_ = std::make_unique<RailProgressBossSpawnService>();
		occurrenceBoss_->OnSceneLoaded(*sceneContext_);

		EnemyEngagementParams params{};
		params.ndcPad        = 0.05f;  // 画面端の余白
		params.minExposeSec  = 0.20f;  // 0.2秒以上映ってから有効
		params.maxEngageDist = 120.0f; // 射程
		params.useLOS        = true;   // 遮蔽物チェックON

		enemyEngagement_ = Installers::InstallEnemyEngagement(*sceneContext_,params);

		if(enemyEngagement_) { enemyEngagement_->SetDirectory(enemyBinding_->GetDirectory()); }
	}
	score_ = std::make_unique<ScoreService>();
	score_->Initialize();

	numbersSprite_ = std::make_unique<NumbersSprite>(
		"Textures/Numbers",".png");

	numbersSprite_->Initialize(/*pos*/ {1280.0f - 640.0f,32.0f},
									   /*digitSize*/ {32.0f,32.0f});
	numbersSprite_->SetAlign(NumbersSprite::DigitsAlign::Right);

	// カメラアクション
	cameraTurnAround_ = std::make_unique<CameraTurnAroundAction>();
	wMainCamera_      = sceneContext_->FindFirst<Camera3d>();
	auto cam = wMainCamera_.lock();
	cam->SetFollowTarget(&wPlayer_.lock()->GetWorldTransform());
}

/////////////////////////////////////////////////////////////////////////////////////////
//  更新
/////////////////////////////////////////////////////////////////////////////////////////
void GameScene::Update([[maybe_unused]] float dt) {
	if(enemyBinding_) enemyBinding_->Update(*sceneContext_,dt);
	if(enemyEngagement_) enemyEngagement_->Update(dt);

	auto mainCam = wMainCamera_.lock();
	//if(cameraTurnAround_) cameraTurnAround_->Update(mainCam.get(),dt);

	// 敵弾コンテナ更新
	enemyBulletContainer_->Update(dt);
	enemyBulletContainer_->AlwaysUpdate(dt);

	// Railの進み具合でボスを発生させる
	occurrenceBoss_->BossSpawnByRailProgress();

	// UI 更新など
	if(attackSprite_) attackSprite_->Update();

	// 衝突判定
	CollisionManager::GetInstance()->UpdateCollisionAllCollider();

	// ---- スコア更新とUI反映 ----
	if(score_) {
		score_->Update();
		if(numbersSprite_) {
			numbersSprite_->SetValue(score_->GetTotal());
			numbersSprite_->Update();
		}
	}

	// ===== クリア／ゲームオーバー条件 =====
	auto player = wPlayer_.lock();
	wBoss_      = sceneContext_->FindFirst<Boss>();
	auto boss   = wBoss_.lock();

	// プレイヤーの死亡
	if(player && !player->GetIsAlive()) {
		transitionRequestor_->RequestSceneChange(SceneType::DEFEAT);
		return;
	}

	
	// ボスは「存在しているときだけ」死亡判定
	// 未スポーン or 破棄済みのフレームでは何もしない
	if (boss && !boss->GetIsAlive()) {

		SceneTransitionPayload payload{};
		payload.score = score_ ? score_->GetTotal() : 0;

		transitionRequestor_->RequestSceneChange(SceneType::CLEAR, payload);
		return;
	}
}

/////////////////////////////////////////////////////////////////////////////////////////
//  描画
/////////////////////////////////////////////////////////////////////////////////////////
void GameScene::Draw(ID3D12GraphicsCommandList* cmdList,
					 PipelineService*           psoService,
					 RenderTargetType           type) {

	auto player = wPlayer_.lock();
	
	BaseScene::Draw(cmdList,psoService,type);
	// 既存のスプライト登録
	if(numbersSprite_) { for(auto* sp : numbersSprite_->GetSpritesRaw()) { spriteRenderer_->Register(sp); } }

	// プレイヤーが持つ追加スプライトを登録
	if(player) { for(auto& sp : player->GetAllSprites()) { if(sp) spriteRenderer_->Register(sp); } }
	if(attackSprite_) { spriteRenderer_->Register(attackSprite_.get()); }

}

/////////////////////////////////////////////////////////////////////////////////////////
//  終了処理
/////////////////////////////////////////////////////////////////////////////////////////
void GameScene::CleanUp() {
	// サービス側の後始末
	if(enemyBinding_) {
		enemyBinding_->OnSceneCleared(*sceneContext_);
		enemyBinding_.reset();
	}

	if(score_) {
		score_->Shutdown();
		score_.reset();
	}

	// シーン内オブジェクト/コライダ掃除
	sceneContext_->GetObjectLibrary()->Clear();
	CollisionManager::GetInstance()->ClearColliders();
}