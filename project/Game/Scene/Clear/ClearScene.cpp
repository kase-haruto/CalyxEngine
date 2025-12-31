#include "ClearScene.h"
#include <Engine/Objects/2D/NumbersSprite/NumbersSprite.h>

// scene
#include "Game/3d/GameCamera/RailCamera.h"
#include <Game/Scene/Utility/SceneTypeUtil.h>

#include <Engine/Foundation/Input/Input.h>
#include <Engine/Application/System/Enviroment.h>
#include <Engine/Collision/CollisionManager.h>
#include <Engine/Scene/Serializer/SceneSerializer.h>
#include <Engine/Scene/System/SceneManager.h>
#include <Game/Scene/Transition/ResultTransitionPayload.h>

/////////////////////////////////////////////////////////////////////////////////////////
//  ctor / dtor
/////////////////////////////////////////////////////////////////////////////////////////
ClearScene::ClearScene() {
	SetSceneName("Clear");
}
ClearScene::~ClearScene() = default;

/////////////////////////////////////////////////////////////////////////////////////////
//  初期化
/////////////////////////////////////////////////////////////////////////////////////////
void ClearScene::Initialize() {
	// SceneContext 初期化
	sceneContext_->Initialize();

	// シーンデータ読み込み
	SceneSerializer::Load(*sceneContext_, "Resources/Assets/Scenes/TitleScene.scene");

	// シーンデータ読み込み
	// ベース初期化
	BaseScene::Initialize();

	clearSprite_   = std::make_unique<Sprite>("Textures/clear.png");
	CalyxMath::Vector2 center = kGameSize * 0.5f;
	clearSprite_->SetSize(kGameSize);
	clearSprite_->SetAnchorPoint(CalyxMath::Vector2(0.5f, 0.5f));
	clearSprite_->SetPosition(center);

	buttonSprite_ = std::make_unique<Sprite>("Textures/button_A.png");
	buttonSprite_->SetAnchorPoint(CalyxMath::Vector2(0.5f, 0.5f));
	buttonSprite_->SetSize(CalyxMath::Vector2(64.0f, 64.0f));
	CalyxMath::Vector2 pos = CalyxMath::Vector2(center.x, 600.0f);
	buttonSprite_->SetPosition(pos);

	// result:用スプライト
	resultScoreSprite_ = std::make_unique<Sprite>("Textures/resultScore.png");
	resultScoreSprite_->SetAnchorPoint(CalyxMath::Vector2(0.0f, 0.5f));
	resultScoreSprite_->SetSize(CalyxMath::Vector2(300.0f, 56.0f));
	resultScoreSprite_->SetPosition(CalyxMath::Vector2(center.x - 400.0f, 320.0f));

	// スコア表示用 NumbersSpriteカクトクスコア
	scoreSprite_ = std::make_unique<NumbersSprite>("Textures/Numbers", ".png");
	scoreSprite_->Initialize(
		{center.x, 320.0f},
		{48.0f, 48.0f} // 桁サイズ
	);
	scoreSprite_->SetAlign(NumbersSprite::DigitsAlign::Center);
	scoreSprite_->SetValue(finalScore_);

	auto cam = sceneContext_->FindFirst<RailCamera>();
	cam->SetSpeed(0.0f);
}
void ClearScene::Update(float dt) {
	blinkTimer += dt;
	if(blinkTimer >= blinkInterval) {
		blinkTimer -= blinkInterval; // オーバー分を残す
		blinkState = !blinkState;
		buttonSprite_->SetIsVisible(blinkState);
	}
	
	if(resultScoreSprite_)resultScoreSprite_->Update();
	if(clearSprite_) clearSprite_->Update();
	if(buttonSprite_) buttonSprite_->Update();
	if(scoreSprite_) scoreSprite_->Update();

	CollisionManager::GetInstance()->UpdateCollisionAllCollider();

	// 遷移
	if(CalyxFoundation::Input::GetInstance()->TriggerGamepadButton(CalyxFoundation::PadButton::A) ||
	   CalyxFoundation::Input::GetInstance()->TriggerKey(DIK_SPACE)) {
		transitionRequestor_->RequestSceneChange(GameSceneUtil::ToSceneId(SceneType::TITLE));
	}
}

/////////////////////////////////////////////////////////////////////////////////////////
//  描画
/////////////////////////////////////////////////////////////////////////////////////////
void ClearScene::Draw(ID3D12GraphicsCommandList* cmdlist,
					  class PipelineService*	 pso,
					  RenderTargetType			 renderTarget) {
	SceneContext* ctx = GetSceneContext();
	if(!ctx) {
		BaseScene::Draw(cmdlist, pso, renderTarget);
		return;
	}
	
	spriteRenderer_->Register(clearSprite_.get());
	if(buttonSprite_) {
		spriteRenderer_->Register(buttonSprite_.get());
	}
	if(scoreSprite_) {
		for(auto* sp : scoreSprite_->GetSpritesRaw()) {
			spriteRenderer_->Register(sp);
		}
	}
	spriteRenderer_->Register(resultScoreSprite_.get());

	BaseScene::Draw(cmdlist, pso, renderTarget);
}

/////////////////////////////////////////////////////////////////////////////////////////
//  終了処理
/////////////////////////////////////////////////////////////////////////////////////////
void ClearScene::CleanUp() {
	// シーン内オブジェクト/コライダ掃除
	sceneContext_->GetObjectLibrary()->Clear();
	CollisionManager::GetInstance()->ClearColliders();
}
void ClearScene::LoadAssets() {
}

/////////////////////////////////////////////////////////////////////////////////////////
//  シーン遷移ペイロード設定
/////////////////////////////////////////////////////////////////////////////////////////
void ClearScene::OnPayload(std::unique_ptr<CalyxScene::IScenePayload> payload) {
	if (!payload) return;

	// 自分が知っている型にだけキャストする
	if (auto* p = static_cast< ResultTransitionPayload*>(payload.get())) {
		finalScore_ = p->score;
	}
}