#include "ClearScene.h"
#include <Engine/Objects/2D/NumbersSprite/NumbersSprite.h>

// scene
#include <Engine/Application/Input/Input.h>
#include <Engine/Application/System/Enviroment.h>
#include <Engine/Collision/CollisionManager.h>
#include <Engine/Scene/Serializer/SceneSerializer.h>
#include <Engine/Scene/System/SceneManager.h>

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
	// ベース初期化
	BaseScene::Initialize();

	Vector2 center = kGameSize * 0.5f;
	buttonSprite_ = std::make_unique<Sprite>("Textures/button_A.png");
	buttonSprite_->SetAnchorPoint(Vector2(0.5f, 0.5f));
	buttonSprite_->SetSize(Vector2(64.0f, 64.0f));
	Vector2 pos = Vector2(center.x, 600.0f);
	buttonSprite_->SetPosition(pos);


	// スコア表示用 NumbersSprite
	scoreSprite_ = std::make_unique<NumbersSprite>("Textures/Numbers", ".png");
	scoreSprite_->Initialize(
		{center.x, 320.0f},      // 表示位置はお好みで
		{48.0f, 48.0f}           // 桁サイズ
	);
	scoreSprite_->SetAlign(NumbersSprite::DigitsAlign::Center);
	scoreSprite_->SetValue(finalScore_); 
}
void ClearScene::Update(float dt) {
	blinkTimer += dt;
	if (blinkTimer >= blinkInterval){
		blinkTimer -= blinkInterval;    // オーバー分を残す
		blinkState = !blinkState;
		buttonSprite_->SetIsVisible(blinkState);
	}
	
	if (buttonSprite_)  buttonSprite_->Update();
	if (scoreSprite_)   scoreSprite_->Update();
	
	CollisionManager::GetInstance()->UpdateCollisionAllCollider();

	// 遷移
	if(Input::GetInstance()->TriggerGamepadButton(PadButton::A)||
		Input::GetInstance()->TriggerKey(DIK_SPACE)) {
		transitionRequestor_->RequestSceneChange(SceneType::TITLE);
	}
}

/////////////////////////////////////////////////////////////////////////////////////////
//  描画
/////////////////////////////////////////////////////////////////////////////////////////
void ClearScene::Draw(ID3D12GraphicsCommandList* cmdlist,
					  class PipelineService* pso,
					  RenderTargetType renderTarget) {
	SceneContext* ctx = GetSceneContext();
	if (!ctx) {
		BaseScene::Draw(cmdlist, pso, renderTarget);
		return;
	}

	if (buttonSprite_) {
		spriteRenderer_->Register(buttonSprite_.get());
	}
	if (scoreSprite_) {
		for (auto* sp : scoreSprite_->GetSpritesRaw()) {
			spriteRenderer_->Register(sp);
		}
	}

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

void ClearScene::SetPayload(const SceneTransitionPayload& payload) {
	finalScore_ = payload.score;
}