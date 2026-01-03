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

	// CLEAR LOGO
	clearLogo_ = std::make_unique<ClearLogoHud>();
	clearLogo_->Initialize();

	// TOTAL SCORE
	totalScore_ = std::make_unique<NumbersSprite>("Textures/Numbers", ".png");
	totalScore_->Initialize({center.x + 200, 340}, {64, 64});
	totalScore_->SetAlign(NumbersSprite::DigitsAlign::Center);
	totalScore_->SetValue(payload.score);

	// ENEMY RESULT（左）
	float y = 320.0f;
	for(const auto& e : payload.results) {
		EnemyRow row;

		row.icon = std::make_unique<Calyx2D::SpriteObject2d>();
		row.icon->Initialize("Textures/white1x1.png");
		row.icon->SetScale({48, 48});
		row.icon->SetPosition({center.x - 320, y});

		row.count = std::make_unique<NumbersSprite>("Textures/Numbers", ".png");
		row.count->Initialize({center.x - 240, y}, {32, 32});
		row.count->SetValue(e.count);

		enemyRows_.push_back(std::move(row));
		y += 56.0f;
	}

	// CONTINUE
	continueIcon_ = std::make_unique<Calyx2D::SpriteObject2d>();
	continueIcon_->Initialize("Textures/white1x1.png");
	continueIcon_->SetScale({96, 96});
	continueIcon_->SetPosition({center.x, 600});
}

//////////////////////////////////////////////////////////////////////////
//	更新
//////////////////////////////////////////////////////////////////////////
void ResultOverlay::Update(float dt) {
	timer_ += dt;

	if(clearLogo_) clearLogo_->Update(dt);
	if(continueIcon_) continueIcon_->Update(dt);
	if(totalScore_) totalScore_->Update();

	// 1秒後に CONTINUE 表示
	if(timer_ > 1.0f) {
		showContinue_ = true;
		continueIcon_->GetSprite()->SetIsVisible(showContinue_);
	}
}

//////////////////////////////////////////////////////////////////////////
//	描画
//////////////////////////////////////////////////////////////////////////
void ResultOverlay::Draw(class SpriteRenderer* renderer) const {
	if(clearLogo_) clearLogo_->Draw(renderer);

	for(auto& row : enemyRows_) {
		row.icon->Draw(renderer);
		for(auto* sp : row.count->GetSpritesRaw()) {
			renderer->Register(sp);
		}
	}

	if(totalScore_) {
		for(auto* sp : totalScore_->GetSpritesRaw()) {
			renderer->Register(sp);
		}
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

	ImGui::End();
}