#include "ResultOverlay.h"

/*===========================================================================
 *	include space
 * ========================================================================*/
#include "Engine/Application/System/Enviroment.h"
#include "Engine/Renderer/Sprite/SpriteRenderer.h"

//////////////////////////////////////////////////////////////////////////
//	コンストラクタ / デストラクタ
//////////////////////////////////////////////////////////////////////////
ResultOverlay::ResultOverlay()	= default;
ResultOverlay::~ResultOverlay() = default;

//////////////////////////////////////////////////////////////////////////
//	初期化
//////////////////////////////////////////////////////////////////////////
void ResultOverlay::Initialize(const ResultTransitionPayload& payload) {
	const CalyxMath::Vector2 center = kGameSize * 0.5f;
	//==============================
	// 背景暗転フィルタ
	//==============================
	dimFilter_ = std::make_unique<ScreenDimFilter>();
	dimFilter_->Initialize(kGameSize);

	// Result開始と同時にフェードイン
	dimFilter_->StartFadeIn();

	// CLEAR LOGO
	clearLogo_ = std::make_unique<ClearLogoHud>();
	clearLogo_->Initialize();

	scoreHud_ = std::make_unique<ScoreResultHud>();
	scoreHud_->Initialize();
	scoreHud_->SetScore(payload.score);

	enemyHud_ = std::make_unique<EnemyResultHud>();
	enemyHud_->Initialize(payload);

	// CONTINUE
	continueIcon_ = std::make_unique<Calyx2D::SpriteObject2d>();
	continueIcon_->Initialize("Textures/white1x1.png");
	continueIcon_->SetScale({96, 96});
	continueIcon_->SetPosition({center.x, 600});

	// 背景オブジェクト
	bkObject_ = std::make_unique<ResultBackgroundObject>();
	bkObject_->Initialize("Textures/ResultHud/bkTexture.png");
}

//////////////////////////////////////////////////////////////////////////
//	更新
//////////////////////////////////////////////////////////////////////////
void ResultOverlay::Update(float dt) {
	timer_ += dt;
	if(dimFilter_)    dimFilter_->Update(dt);

	// フェードが終わったら各hud更新
	if(dimFilter_&&dimFilter_->IsFinished()) {
		bkObject_->Update(dt);
		if(clearLogo_) clearLogo_->Update(dt);
		if(continueIcon_) continueIcon_->Update(dt);
		if(scoreHud_) scoreHud_->Update(dt);
		if(enemyHud_) enemyHud_->Update(dt);

		// 1秒後に CONTINUE 表示
		if(timer_ > 1.0f) {
			showContinue_ = true;
			continueIcon_->GetSprite()->SetIsVisible(showContinue_);
		}
	}
}

//////////////////////////////////////////////////////////////////////////
//	描画
//////////////////////////////////////////////////////////////////////////
void ResultOverlay::Draw(class SpriteRenderer* renderer) const {
	//==============================
	// 背景暗転（最背面）
	//==============================
	if(dimFilter_) {
		dimFilter_->Draw(renderer);
	}

	if(bkObject_) {
		bkObject_->Draw(renderer);
	}

	if(clearLogo_) clearLogo_->Draw(renderer);

	if(enemyHud_) {
		enemyHud_->Draw(renderer);
	}

	if(scoreHud_) {
		scoreHud_->Draw(renderer);
	}

	if(continueIcon_) {
		continueIcon_->Draw(renderer);
	}
}

void ResultOverlay::ShowGUi() {
	ImGui::Begin("Result");

	// クリアロゴ
	if(ImGui::CollapsingHeader("clearLogo")) {
		clearLogo_->ShowGui();
	}

	// スコアHUD
	if(ImGui::CollapsingHeader("scoreHud")) {
		scoreHud_->ShowGui();
	}

	// 敵撃破数HUD
	if(ImGui::CollapsingHeader("enemyHud")) {
		enemyHud_->ShowGui();
	}

	// 背景オブジェクト
	if(ImGui::CollapsingHeader("bkObject")) {
		bkObject_->ShowGui();
	}

	ImGui::End();
}