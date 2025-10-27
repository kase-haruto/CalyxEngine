#include "DefeatScene.h"

// scene
#include <Engine/Application/Input/Input.h>
#include <Engine/Application/System/Enviroment.h>
#include <Engine/Collision/CollisionManager.h>
#include <Engine/Scene/Serializer/SceneSerializer.h>
#include <Engine/Scene/System/SceneManager.h>

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

	defeatSprite_  = std::make_unique<Sprite>("Textures/defeat.png");
	Vector2 center = kGameSize * 0.5f;
	defeatSprite_->SetSize(kGameSize);
	defeatSprite_->SetAnchorPoint(Vector2(0.5f, 0.5f));
	defeatSprite_->SetPosition(center);

	buttonSprite = std::make_unique<Sprite>("Textures/button_A.png");
	buttonSprite->SetAnchorPoint(Vector2(0.5f, 0.5f));
	buttonSprite->SetSize(Vector2(64.0f, 64.0f));
	Vector2 pos = Vector2(center.x, 600.0f);
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
	if(Input::GetInstance()->TriggerGamepadButton(PadButton::A)) {
		transitionRequestor_->RequestSceneChange(SceneType::TITLE);
	}
}

/////////////////////////////////////////////////////////////////////////////////////////
//  描画
/////////////////////////////////////////////////////////////////////////////////////////
void DefeatScene::Draw(ID3D12GraphicsCommandList* cmdlist, class PipelineService* pso, RenderTargetType renderTarget) {
	SceneContext* ctx = GetSceneContext();
	if(!ctx) {
		BaseScene::Draw(cmdlist, pso, renderTarget);
		return;
	}

	spriteRenderer_->Register(defeatSprite_.get());
	spriteRenderer_->Register(buttonSprite.get());

	BaseScene::Draw(cmdlist, pso, renderTarget);
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