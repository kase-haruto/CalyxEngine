

/////////////////////////////////////////////////////////////////////////////////////////
//	include
/////////////////////////////////////////////////////////////////////////////////////////

// scene
#include "Game/Scene/Details/SceneType.h"
#include "Game/Scene/Utility/SceneTypeUtil.h"
#include <Game/DemoPlayer/DemoPlayer.h>
#include <Game/Scene/Select/SelectScene.h>
#include <Game\Scene\Transition\TransitionPayload.h>

// engine
#include <Engine/Collision/CollisionManager.h>
#include <Engine/Foundation/Input/Input.h>
#include <Engine/Foundation/Utility/Func/MyFunc.h>
#include <Engine/Scene/Context/SceneContext.h>
#include <Engine/Scene/Serializer/SceneSerializer.h>
// lib

/////////////////////////////////////////////////////////////////////////////////////////
//	コンストラクタ/デストラクタ
/////////////////////////////////////////////////////////////////////////////////////////
SelectScene::SelectScene() {
	// シーン名を設定
	BaseScene::SetSceneName("SelectScene");
}

/////////////////////////////////////////////////////////////////////////////////////////
//	アセットのロード
/////////////////////////////////////////////////////////////////////////////////////////
void SelectScene::LoadAssets() {}

/////////////////////////////////////////////////////////////////////////////////////////
//	初期化処理
/////////////////////////////////////////////////////////////////////////////////////////
void SelectScene::Initialize() {
	sceneContext_->Initialize();

	sceneContext_->SetSceneName("SelectScene");

	BaseScene::Initialize();

	SceneSerializer::Load(*sceneContext_, "Resources/Assets/Scenes/Select.scene");

	LoadAssets();

	//=========================
	// グラフィック関連
	//=========================
	pauseBg_ = std::make_unique<Sprite>("Textures/uvChecker.dds");
	pauseBg_->Initialize({0.0f, 0.0f}, {640.0f, 360.0f});
	pauseBg_->SetColor({0.0f, 0.0f, 1.0f, 1.0f});
	pauseBg_->Update();

	transitionControl_ = std::make_unique<TransitionControl>();
	if(preType_ == SceneType::TITLE) {
		transitionControl_->Initialize("Textures/Transition/wave.png", "Textures/Transition/wave.png", "Textures/Transition/wave.png");
	} else {
		transitionControl_->Initialize("Textures/uvChecker.dds", "Textures/uvChecker.dds", "Textures/uvChecker.dds");
	}
	// シーンタイプに基づいて自動で演出をセット
	transitionControl_->SetAutoPresetFromPrevious(preType_, SceneType::SELECT);
	transitionControl_->StartOpening(0.75f, [this]() {
		IsOpening_ = false;
		transitionControl_->SetTexturePlate1("Textures/uvChecker.dds");
		transitionControl_->SetTexturePlate2("Textures/uvChecker.dds");
	});
	IsOpening_ = true;
	IsPhase_   = false;
}

/////////////////////////////////////////////////////////////////////////////////////////
//	更新処理
/////////////////////////////////////////////////////////////////////////////////////////
void SelectScene::Update([[maybe_unused]] float dt) {

	transitionControl_->Update(dt);

	if(IsPhase_ || IsOpening_) {
		return;
	}

	SelectUpdate(dt);

	if(CalyxFoundation::Input::TriggerKey(DIK_7)) {
		IsPhase_ = true;
		transitionControl_->SetAutoPreset(SceneType::SELECT, SceneType::TEST);
		transitionControl_->StartClosing(0.5f, [this]() {
			transitionRequestor_->RequestSceneChange(GameSceneUtil::ToSceneId(SceneType::TEST));
		});
	}

	// 衝突判定
	CollisionManager::GetInstance()->UpdateCollisionAllCollider();
}

void SelectScene::Draw(ID3D12GraphicsCommandList* cmdList, PipelineService* psoService, IRenderTarget* rt) {

	//========================================================//
	//	sprite の 登録
	//========================================================//
	spriteRenderer_->Register(pauseBg_.get());
	transitionControl_->Draw(spriteRenderer_.get());

	// シーン上のオブジェクトの描画
	BaseScene::Draw(cmdList, psoService, rt);
}

void SelectScene::CleanUp() {
	// 3Dオブジェクトの描画を終了
	sceneContext_->GetObjectLibrary()->Clear();
	CollisionManager::GetInstance()->ClearColliders();
}

void SelectScene::SelectUpdate(float) {
	const int maxStages = 5;

	// 左右入力によるステージ選択
	if(CalyxFoundation::Input::TriggerKey(DIK_A) || CalyxFoundation::Input::TriggerKey(DIK_LEFT) ||
	   CalyxFoundation::Input::TriggerGamepadButton(CalyxFoundation::PadButton::DPAD_LEFT)) {
		selectedIndex_ = (selectedIndex_ - 1 + maxStages) % maxStages;
	}
	if(CalyxFoundation::Input::TriggerKey(DIK_D) || CalyxFoundation::Input::TriggerKey(DIK_RIGHT) ||
	   CalyxFoundation::Input::TriggerGamepadButton(CalyxFoundation::PadButton::DPAD_RIGHT)) {
		selectedIndex_ = (selectedIndex_ + 1) % maxStages;
	}

	// 決定操作
	if(CalyxFoundation::Input::TriggerKey(DIK_SPACE) || CalyxFoundation::Input::TriggerGamepadButton(CalyxFoundation::PadButton::A)) {
		gamePayload_ = BuildGamePayload(selectedIndex_);
		IsPhase_	 = true;
		transitionControl_->SetAutoPreset(SceneType::SELECT, SceneType::TEST);
		transitionControl_->StartClosing(0.5f, [this]() {
			transitionRequestor_->RequestSceneChange(GameSceneUtil::ToSceneId(SceneType::TEST), std::move(gamePayload_));
		});
	}

	// 選択中のステージに応じて背景色を変更（デバッグ用フィードバック）
	switch(selectedIndex_) {
	case 0: pauseBg_->SetColor({0.0f, 0.0f, 1.0f, 1.0f}); break;
	case 1: pauseBg_->SetColor({1.0f, 0.0f, 0.0f, 1.0f}); break;
	case 2: pauseBg_->SetColor({0.0f, 1.0f, 0.0f, 1.0f}); break;
	case 3: pauseBg_->SetColor({1.0f, 1.0f, 0.0f, 1.0f}); break;
	case 4: pauseBg_->SetColor({1.0f, 0.0f, 1.0f, 1.0f}); break;
	}
	pauseBg_->Update();
}

void SelectScene::PhaseUpdate(float) {

}

std::unique_ptr<GameTransitionPayload> SelectScene::BuildGamePayload(int num) {
	auto payload = std::make_unique<GameTransitionPayload>();
	payload->stageNum_ = num;
	payload->type_ = SceneType::SELECT;
	return payload;
}

void SelectScene::OnPayload(std::unique_ptr<CalyxEngine::IScenePayload> payload) {
	if(!payload) return;

	// 自分が知っている型にだけキャストする
	if(auto* p = static_cast<TransitionPayload*>(payload.get())) {
		preType_ = p->type;
	}
}
