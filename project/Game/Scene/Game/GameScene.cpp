#include "GameScene.h"

/////////////////////////////////////////////////////////////////////////////////////////
//  include
/////////////////////////////////////////////////////////////////////////////////////////

// scene
#include <Engine/Scene/System/SceneManager.h>

// engine
#include <Engine/Collision/CollisionManager.h>
#include <Engine/Foundation/Input/Input.h>
#include <Engine/Graphics/Camera/Action/CameraTurnAroundAction.h>
#include <Engine/Objects/2D/NumbersSprite/NumbersSprite.h>
#include <Engine/Objects/3D/Actor/SceneObjectManager.h>
#include <Engine/Scene/Serializer/SceneSerializer.h>
#include <Engine/Scene/Utility/SceneUtility.h>

// game
#include "Engine/Application/System/Enviroment.h"
#include "Game/Scene/Transition/ResultTransitionPayload.h"
#include "Game/Scene/Utility/SceneTypeUtil.h"

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
	SceneSerializer::Load(*sceneContext_, "Resources/Assets/Scenes/GameScene.scene");

	// ベース初期化
	BaseScene::Initialize();

	// アセット読み込み
	LoadAssets();

	// 弾の登録
	BulletRegistrar::RegisterAll();

	if(auto* ctx = SceneContext::Current()) {
		if(auto ground = ctx->FindObjectByName<BaseGameObject>("field")) {
			ground->SetEnableRaycast(false);
		}
	}

	// UI
	{
		shootUI_				  = std::make_unique<Sprite>("Textures/UI/shootUI.png");
		aimUI_					  = std::make_unique<Sprite>("Textures/UI/aimUI.png");
		avoidanceUI_			  = std::make_unique<Sprite>("Textures/UI/avoidanceUI.png");
		CalyxMath::Vector2 uiSize = {128.0f, 64.0f};
		shootUI_->SetSize(uiSize);
		aimUI_->SetSize(uiSize);
		avoidanceUI_->SetSize(uiSize);

		// 左端中央
		shootUI_->SetAnchorPoint(CalyxMath::Vector2(0.0f, 0.5f));
		aimUI_->SetAnchorPoint(CalyxMath::Vector2(0.0f, 0.5f));
		avoidanceUI_->SetAnchorPoint(CalyxMath::Vector2(0.0f, 0.5f));
		float			   space = 32.0f;
		CalyxMath::Vector2 base	 = {100.0f, (kGameHeight / 2.0f) - space};
		Sprite*			   uis[] = {shootUI_.get(), aimUI_.get(), avoidanceUI_.get()};

		for(size_t i = 0; i < std::size(uis); ++i) {
			if(uis[i]) {
				CalyxMath::Vector2 pos = base;
				pos.y += static_cast<float>(i) * space; // 200ずつ下にずらす
				uis[i]->SetPosition(pos);
			}
		}
	}

	// プレイヤー基本セットアップ
	{
		auto			player = sceneContext_->FindFirst<Player>();
		PlayerInstaller installer;
		installer.InstallPlayer(player);
		wPlayer_ = player; // Draw でスプライト拾うために持っておく
	}

	// 敵セットアップ

	{
		// 敵弾コンテナ
		enemyBulletContainer_ = std::make_unique<EnemyBulletContainer>("EnemyBulletContainer");

		enemyBinding_ = std::make_unique<EnemyRuntimeBindingService>();
		enemyBinding_->OnSceneLoaded(*sceneContext_, enemyBulletContainer_.get());

		occurrenceBoss_ = std::make_unique<RailProgressBossSpawnService>();
		occurrenceBoss_->OnSceneLoaded(*sceneContext_);

		EnemyEngagementParams params{};
		params.ndcPad		 = 0.05f;  // 画面端の余白
		params.minExposeSec	 = 0.20f;  // 0.2秒以上映ってから有効
		params.maxEngageDist = 120.0f; // 射程
		params.useLOS		 = true;   // 遮蔽物チェックON

		enemyEngagement_ = Installers::InstallEnemyEngagement(*sceneContext_, params);

		if(enemyEngagement_) {
			enemyEngagement_->SetDirectory(enemyBinding_->GetDirectory());
		}
	}
	score_ = std::make_unique<ScoreService>();
	score_->Initialize();

	numbersSprite_ = std::make_unique<NumbersSprite>(
		"Textures/Numbers", ".png");
	CalyxMath::Vector2 scoreSpritePos = {100.0f, 630.0f};
	numbersSprite_->Initialize(/*pos*/ {kGameWidth - scoreSpritePos.x, scoreSpritePos.y},
							   /*digitSize*/ {32.0f, 32.0f});
	numbersSprite_->SetAlign(NumbersSprite::DigitsAlign::Right);

	// リザルトオーバーレイ初期化
	resultOverlay_ = std::make_unique<ResultOverlay>();

	updateFunc_ = &GameScene::PlayingUpdate;
}

/////////////////////////////////////////////////////////////////////////////////////////
//  更新
/////////////////////////////////////////////////////////////////////////////////////////
void GameScene::Update(float dt) {
	// 更新関数の呼び出し
	if(updateFunc_) {
		(this->*updateFunc_)(dt);
	}
}

/////////////////////////////////////////////////////////////////////////////////////////
//  ゲームプレイ中の更新
/////////////////////////////////////////////////////////////////////////////////////////
void GameScene::PlayingUpdate(float dt) {
	if(enemyBinding_) enemyBinding_->Update(*sceneContext_, dt);
	if(enemyEngagement_) enemyEngagement_->Update(dt);

	auto mainCam = wMainCamera_.lock();
	// 敵弾コンテナ更新
	enemyBulletContainer_->Update(dt);
	enemyBulletContainer_->AlwaysUpdate(dt);

	// Railの進み具合でボスを発生させる
	occurrenceBoss_->BossSpawnByRailProgress();

	// UI 更新など
	shootUI_->Update();
	aimUI_->Update();
	avoidanceUI_->Update();
	// スコア更新とスプライト反映
	ScoreUpdate();

	// 衝突判定
	CollisionManager::GetInstance()->UpdateCollisionAllCollider();

	// ===== クリア／ゲームオーバー条件 =====
	auto player = wPlayer_.lock();
	wBoss_		= sceneContext_->FindFirst<Boss>();
	auto boss	= wBoss_.lock();

	// プレイヤーの死亡
	if(player && !player->GetIsAlive()) {
		transitionRequestor_->RequestSceneChange(GameSceneUtil::ToSceneId(SceneType::DEFEAT));
		return;
	}

	if(boss && !boss->GetIsAlive()) {
		EnterResultPhase();
		return;
	}

	// 一応タイトル戻るよう
	if(CalyxFoundation::Input::GetInstance()->TriggerGamepadButton(CalyxFoundation::PadButton::START)) {
		transitionRequestor_->RequestSceneChange(GameSceneUtil::ToSceneId(SceneType::TITLE));
	}
}

/////////////////////////////////////////////////////////////////////////////////////////
//  リザルト用更新
/////////////////////////////////////////////////////////////////////////////////////////
void GameScene::ResultUpdate(float dt) {
	if(resultOverlay_) {
		resultOverlay_->Update(dt);
	}
}

/////////////////////////////////////////////////////////////////////////////////////////
//  描画
/////////////////////////////////////////////////////////////////////////////////////////
void GameScene::Draw(ID3D12GraphicsCommandList* cmdList,
					 PipelineService*			psoService,
					 RenderTargetType			type) {
	BaseScene::Draw(cmdList, psoService, type);

	// ingameの更新の時
	if(updateFunc_ == &GameScene::PlayingUpdate) {
		auto player = wPlayer_.lock();

		// 既存のスプライト登録
		if(numbersSprite_) {
			for(auto* sp : numbersSprite_->GetSpritesRaw()) {
				spriteRenderer_->Register(sp);
			}
		}

		// プレイヤーが持つ追加スプライトを登録
		if(player) {
			for(auto& sp : player->GetAllSprites()) {
				if(sp) spriteRenderer_->Register(sp);
			}
		}
		if(shootUI_) {
			spriteRenderer_->Register(shootUI_.get());
		}
		if(aimUI_) {
			spriteRenderer_->Register(aimUI_.get());
		}
		if(avoidanceUI_) {
			spriteRenderer_->Register(avoidanceUI_.get());
		}

		wBoss_	  = sceneContext_->FindFirst<Boss>();
		auto boss = wBoss_.lock();
		if(boss) {
			for(auto& sp : boss->GetAllSprites()) {
				if(sp) spriteRenderer_->Register(sp);
			}
		}
	} else {
		resultOverlay_->Draw(spriteRenderer_.get());
	}
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

std::unique_ptr<ResultTransitionPayload> GameScene::BuildResultPayload() const {
	auto payload = std::make_unique<ResultTransitionPayload>();

	if(score_) {
		payload->score = score_->GetTotal();
		for(const auto& [kind, stat] : score_->GetEnemyStats()) {
			ResultTransitionPayload::ResultEntry entry{};
			entry.kind	= kind;
			entry.count = stat.count;
			entry.score = stat.score;
			payload->results.push_back(entry);
		}
	}

	return payload;
}

void GameScene::EnterResultPhase() {
	resultPayload_ = BuildResultPayload();

	// リザルトオーバーレイ初期化
	resultOverlay_ = std::make_unique<ResultOverlay>();
	resultOverlay_->Initialize(*resultPayload_);

	// 更新パスをへんこう
	updateFunc_ = &GameScene::ResultUpdate;
}

void GameScene::ScoreUpdate() const {
	// ---- スコア更新とUI反映 ----
	if(score_) {
		score_->Update();
		if(numbersSprite_) {
			numbersSprite_->SetValue(score_->GetTotal());
			numbersSprite_->Update();
		}
	}
}
