#include "GameScene.h"

/////////////////////////////////////////////////////////////////////////////////////////
//  include
/////////////////////////////////////////////////////////////////////////////////////////

// scene
#include <Engine/Scene/System/SceneManager.h>

// engine
#include <Engine/Application/Input/Input.h>
#include <Engine/Objects/3D/Actor/SceneObjectManager.h>
#include <Engine/Collision/CollisionManager.h>
#include <Engine/Scene/Utility/SceneUtility.h>
#include <Engine/Scene/Serializer/SceneSerializer.h>
// game
#include <Game/Installer/Enemy/EnemyEngagementInstaller.h>
#include <Game/3dObject/Actor/Bullet/Register/BulletRegistrar.h>
#include <Game/Installer/Player/PlayerInstaller.h>



/////////////////////////////////////////////////////////////////////////////////////////
//  ctor / dtor
/////////////////////////////////////////////////////////////////////////////////////////
GameScene::GameScene() {
	SetSceneName("Game");
}

/////////////////////////////////////////////////////////////////////////////////////////
//  アセットのロード
/////////////////////////////////////////////////////////////////////////////////////////
void GameScene::LoadAssets() {
	// 必要ならここでモデル/テクスチャ等をプリロード
}

/////////////////////////////////////////////////////////////////////////////////////////
//  初期化処理
/////////////////////////////////////////////////////////////////////////////////////////
void GameScene::Initialize() {
	// SceneContext 初期化
	sceneContext_->Initialize();

	// シーンデータ読み込み
	SceneSerializer::Load(*sceneContext_, "Resources/Assets/Scenes/GameScene.scene");

	// ベース初期化
	BaseScene::Initialize();

	// アセット読み込み
	LoadAssets();

	// 弾の登録
	BulletRegistrar::RegisterAll();

	if (auto* ctx = SceneContext::Current()) {
		if (auto ground = ctx->FindObjectByName<BaseGameObject>("field")) {
			ground->SetEnableRaycast(false);
		}
	}

	// UI（攻撃状態）
	attackSprite_ = std::make_unique<Sprite>("Textures/attackUI.png");
	{
		Vector2 attackUiPos = Vector2(1280.0f * 0.5f, 720.0f - 100.0f);
		Vector2 attackUiSize = Vector2(128.0f, 128.0f);
		attackSprite_->Initialize(attackUiPos, attackUiSize);
		attackSprite_->SetAnchorPoint(Vector2(0.5f, 0.5f));
	}

	// プレイヤー基本セットアップ（Move/Install など）
	{
		auto player = sceneContext_->FindFirst<Player>();
		PlayerInstaller installer;
		installer.InstallPlayer(player);
		wPlayer_ = player; // Draw でスプライト拾うために持っておく
	}

	enemyBinding_ = std::make_unique<EnemyRuntimeBindingService>();
	enemyBinding_->OnSceneLoaded(*sceneContext_);

	occurrenceBoss_ = std::make_unique<RailProgressBossSpawnService>();
	occurrenceBoss_->OnSceneLoaded(*sceneContext_);

	EnemyEngagementParams params{};
	params.ndcPad = 0.05f;			// 画面端の余白
	params.minExposeSec = 0.20f;	// 0.2秒以上映ってから有効
	params.maxEngageDist = 120.0f;	// 射程
	params.useLOS = true;			// 遮蔽物チェックON

	enemyEngagement_ = Installers::InstallEnemyEngagement(*sceneContext_, params);

	if (enemyEngagement_) {
		enemyEngagement_->SetDirectory(enemyBinding_->GetDirectory()); // ★ここが肝
	}
}

/////////////////////////////////////////////////////////////////////////////////////////
//  更新
/////////////////////////////////////////////////////////////////////////////////////////
void GameScene::Update([[maybe_unused]] float dt) {
	// ランタイム配線サービスの更新
	if (enemyBinding_)    enemyBinding_->Update(*sceneContext_, dt);
	if (enemyEngagement_) enemyEngagement_->Update(dt);

	//Railの進み具合でボスを発生させる
	occurrenceBoss_->BossSpawnByRailProgress();

	// UI 更新など
	if (attackSprite_) attackSprite_->Update();

	// 衝突判定
	CollisionManager::GetInstance()->UpdateCollisionAllCollider();
}

/////////////////////////////////////////////////////////////////////////////////////////
//  描画
/////////////////////////////////////////////////////////////////////////////////////////
void GameScene::Draw(ID3D12GraphicsCommandList* cmdList,
					 PipelineService* psoService,
					 RenderTargetType type) {
	SceneContext* ctx = GetSceneContext();
	if (!ctx) {
		BaseScene::Draw(cmdList, psoService, type);
		return;
	}

	// プレイヤーが持つ追加スプライトを登録
	if (auto player = ctx->FindFirst<Player>()) {
		for (auto& sp : player->GetAllSprites()) {
			if (sp) spriteRenderer_->Register(sp);
		}
	}
	if (attackSprite_) {
		spriteRenderer_->Register(attackSprite_.get());
	}

	BaseScene::Draw(cmdList, psoService, type);
}

/////////////////////////////////////////////////////////////////////////////////////////
//  終了処理
/////////////////////////////////////////////////////////////////////////////////////////
void GameScene::CleanUp() {
	// サービス側の後始末
	if (enemyBinding_) {
		enemyBinding_->OnSceneCleared(*sceneContext_);
		enemyBinding_.reset();
	}

	// シーン内オブジェクト/コライダ掃除
	sceneContext_->GetObjectLibrary()->Clear();
	CollisionManager::GetInstance()->ClearColliders();
}

