

/////////////////////////////////////////////////////////////////////////////////////////
//	include
/////////////////////////////////////////////////////////////////////////////////////////

// scene
#include "Game/Scene/Details/SceneType.h"
#include "Game/Scene/Utility/SceneTypeUtil.h"
#include <Game/DemoPlayer/DemoPlayer.h>
#include <Game/Scene/Title/TitleScene.h>

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
TitleScene::TitleScene() {
	// シーン名を設定
	BaseScene::SetSceneName("TitleScene");
}

/////////////////////////////////////////////////////////////////////////////////////////
//	アセットのロード
/////////////////////////////////////////////////////////////////////////////////////////
void TitleScene::LoadAssets() {}

/////////////////////////////////////////////////////////////////////////////////////////
//	初期化処理
/////////////////////////////////////////////////////////////////////////////////////////
void TitleScene::Initialize() {
	sceneContext_->Initialize();

	sceneContext_->SetSceneName("TitleScene");

	BaseScene::Initialize();

	SceneSerializer::Load(*sceneContext_, "Resources/Assets/Scenes/Title.scene");

	LoadAssets();

	//=========================
	// グラフィック関連
	//=========================
	pauseBg_ = std::make_unique<Sprite>("Textures/uvChecker.dds");
	pauseBg_->Initialize({640.0f, 0.0f}, {320.0f, 180.0f});
	pauseBg_->SetAnchorPoint({0.5f, 0.0f});
	pauseBg_->SetColor({0.0f, 1.0f, 0.0f, 1.0f});
	pauseBg_->Update();

	transitionControl_ = std::make_unique<TransitionControl>();
	if(preType_ == SceneType::SELECT) {
		transitionControl_->Initialize("Textures/uvChecker.dds", "Textures/Transition/wave.png", "Textures/uvChecker.dds");
	} else {
		transitionControl_->Initialize("Textures/uvChecker.dds", "Textures/uvChecker.dds", "Textures/uvChecker.dds");
	}
	// シーンタイプに基づいて自動で演出をセット
	transitionControl_->SetAutoPresetFromPrevious(preType_, SceneType::TITLE);
	transitionControl_->StartOpening(0.5f, [this]() {
		IsOpening_ = false;
		transitionControl_->SetTexturePlate1("Textures/Transition/wave.png");
		transitionControl_->SetTexturePlate2("Textures/Transition/wave.png");
		transitionControl_->SetTexturePlate3("Textures/Transition/wave.png");
	});
	IsOpening_ = true;
	IsPhase_ = false;
}

/////////////////////////////////////////////////////////////////////////////////////////
//	更新処理
/////////////////////////////////////////////////////////////////////////////////////////
void TitleScene::Update([[maybe_unused]] float dt) {

	transitionControl_->Update(dt);

	if(IsPhase_ || IsOpening_) {
		return;
	}

	if(CalyxFoundation::Input::TriggerKey(DIK_SPACE) || CalyxFoundation::Input::TriggerGamepadButton(CalyxFoundation::PadButton::A)) {
		IsPhase_ = true;
		payload_ = BuildNowTypePayload(SceneType::TITLE);
		transitionControl_->SetAutoPreset(SceneType::TITLE, SceneType::SELECT);
		transitionControl_->StartClosing(0.75f, [this]() {
			transitionRequestor_->RequestSceneChange(GameSceneUtil::ToSceneId(SceneType::SELECT), std::move(payload_));
		});
	}

	// 衝突判定
	CollisionManager::GetInstance()->UpdateCollisionAllCollider();
}

void TitleScene::Draw(ID3D12GraphicsCommandList* cmdList, PipelineService* psoService, IRenderTarget* rt) {

	//========================================================//
	//	spriteの登録
	//========================================================//
	spriteRenderer_->Register(pauseBg_.get());

	transitionControl_->Draw(spriteRenderer_.get());

	// シーン上のオブジェクトの描画
	BaseScene::Draw(cmdList, psoService, rt);
}

void TitleScene::CleanUp() {
	// 3Dオブジェクトの描画を終了
	sceneContext_->GetObjectLibrary()->Clear();
	CollisionManager::GetInstance()->ClearColliders();
}

void TitleScene::PhaseUpdate(float) {

}

std::unique_ptr<TransitionPayload> TitleScene::BuildNowTypePayload(SceneType Type) {
	auto payload	   = std::make_unique<TransitionPayload>();
	payload->type = Type;
	return payload;
}

void TitleScene::OnPayload(std::unique_ptr<CalyxEngine::IScenePayload> payload) {
	if(!payload) return;

	// 自分が知っている型にだけキャストする
	if(auto* p = static_cast<TransitionPayload*>(payload.get())) {
		preType_ = p->type;
	}
}