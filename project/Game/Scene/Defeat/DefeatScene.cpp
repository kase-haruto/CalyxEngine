#include "DefeatScene.h"

// scene
#include <Engine/Application/System/Environment.h>
#include <Engine/Collision/CollisionManager.h>
#include <Engine/Foundation/Input/Input.h>
#include <Engine/Scene/Serializer/SceneSerializer.h>
#include <Engine/Scene/System/SceneManager.h>

// game
#include <Game/Scene/Utility/SceneTypeUtil.h>

/////////////////////////////////////////////////////////////////////////////////////////
//  ctor / dtor
/////////////////////////////////////////////////////////////////////////////////////////
DefeatScene::DefeatScene() {
	SetSceneName("Defeat");
}
DefeatScene::~DefeatScene() = default;

/////////////////////////////////////////////////////////////////////////////////////////
//  初期化
/////////////////////////////////////////////////////////////////////////////////////////
void DefeatScene::Initialize() {
	// SceneContext 初期化
	sceneContext_->Initialize();

	// シーンデータ読み込み
	// SceneSerializer::Load(*sceneContext_,"Resources/Assets/Scenes/GameScene.scene");

	// ベース初期化
	BaseScene::Initialize();

	defeatSprite_  = std::make_unique<Sprite>("Textures/defeat.dds");
	CalyxMath::Vector2 center = kGameSize * 0.5f;
	defeatSprite_->SetSize(kGameSize);
	defeatSprite_->SetAnchorPoint(CalyxMath::Vector2(0.5f, 0.5f));
	defeatSprite_->SetPosition(center);

	buttonSprite = std::make_unique<Sprite>("Textures/button_A.dds");
	buttonSprite->SetAnchorPoint(CalyxMath::Vector2(0.5f, 0.5f));
	buttonSprite->SetSize(CalyxMath::Vector2(64.0f, 64.0f));
	CalyxMath::Vector2 pos = CalyxMath::Vector2(center.x, 600.0f);
	buttonSprite->SetPosition(pos);
}
void DefeatScene::Update(float dt) {
	blinkTimer += dt;
	if (blinkTimer >= blinkInterval){
		blinkTimer -= blinkInterval;    // オーバー分を残す
		blinkState = !blinkState;
		buttonSprite->SetIsVisible(blinkState);
	}
	
	defeatSprite_->Update();
	buttonSprite->Update();
	CollisionManager::GetInstance()->UpdateCollisionAllCollider();

	// 遷移
	if(CalyxFoundation::Input::GetInstance()->TriggerGamepadButton(CalyxFoundation::PadButton::A)) {
		transitionRequestor_->RequestSceneChange(GameSceneUtil::ToSceneId(SceneType::TITLE));
	}
}

/////////////////////////////////////////////////////////////////////////////////////////
//  描画
/////////////////////////////////////////////////////////////////////////////////////////
void DefeatScene::Draw(ID3D12GraphicsCommandList* cmdlist, class PipelineService* pso, IRenderTarget* rt) {
	SceneContext* ctx = GetSceneContext();
	if(!ctx) {
		BaseScene::Draw(cmdlist, pso, rt);
		return;
	}

	spriteRenderer_->Register(defeatSprite_.get());
	spriteRenderer_->Register(buttonSprite.get());

	BaseScene::Draw(cmdlist, pso, rt);
}

/////////////////////////////////////////////////////////////////////////////////////////
//  終了処理
/////////////////////////////////////////////////////////////////////////////////////////
void DefeatScene::CleanUp() {
	// シーン内オブジェクト/コライダ掃除
	sceneContext_->GetObjectLibrary()->Clear();
	CollisionManager::GetInstance()->ClearColliders();
}
void DefeatScene::LoadAssets() {
}