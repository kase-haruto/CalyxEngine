#include "TitleScene.h"
/////////////////////////////////////////////////////////////////////////////////////////
//	include
/////////////////////////////////////////////////////////////////////////////////////////

// scene
#include <Engine/Scene/System/SceneManager.h>

// engine
#include <Engine/Graphics/Context/GraphicsGroup.h>
#include <Engine/Application/Input/Input.h>
#include <Engine/Graphics/Camera/Manager/CameraManager.h>
#include <Engine/Objects/3D/Actor/SceneObjectManager.h>
#include <Engine/Collision/CollisionManager.h>
#include <Engine/Assets/Texture/TextureManager.h>
#include <Engine/Application/System/Enviroment.h>
#include <Engine/Scene/Utility/SceneUtility.h>

/////////////////////////////////////////////////////////////////////////////////////////
//	コンストラクタ/デストラクタ
/////////////////////////////////////////////////////////////////////////////////////////
TitleScene::TitleScene() {
	// シーン名を設定
	//IScene::SetSceneName("TitleScene");
	SetSceneName("TitleScene");

}

/////////////////////////////////////////////////////////////////////////////////////////
//	アセットのロード
/////////////////////////////////////////////////////////////////////////////////////////
void TitleScene::LoadAssets() {
}




/////////////////////////////////////////////////////////////////////////////////////////
//	初期化処理
/////////////////////////////////////////////////////////////////////////////////////////
void TitleScene::Initialize() {
	sceneContext_->Initialize();
	LoadAssets();

	CameraManager::GetInstance()->SetType(CameraType::Type_Default);

	//=========================
	// グラフィック関連
	//=========================
	modelField_ = SceneAPI::Instantiate<BaseGameObject>("terrain.obj", "field");
	modelField_->SetScale({ 300.0f,300.0f,300.0f });

	title_ = std::make_unique<Sprite>("Textures/title.png");
	Vector2 pos = {1280.0f,720.0f};
	Vector2 size = Vector2(540.0f*0.8f, 320.0f * 0.8f);
	title_->Initialize(Vector2(pos.x * 0.5f, pos.y * 0.5f), size);

}

void TitleScene::Update() {
	/* カメラ関連更新 ============================*/
	CameraManager::Update();

	skyBox_->Update();

	/* 3dObject ============================*/
	//地面の更新
	modelField_->Update();
	title_->Update();
	/* その他 ============================*/
	sceneContext_->Update();
	CollisionManager::GetInstance()->UpdateCollisionAllCollider();

	if (Input::GetInstance()->TriggerKey(DIK_2)||
		Input::GetInstance()->TriggerGamepadButton(PAD_BUTTON::A)) {
		if (transitionRequestor_) {
			transitionRequestor_->RequestSceneChange(SceneType::PLAY);
		}
	}
}


void TitleScene::CleanUp() {
	// 3Dオブジェクトの描画を終了
	sceneContext_->GetObjectLibrary()->Clear();
	CollisionManager::GetInstance()->ClearColliders();
}

void TitleScene::Draw(ID3D12GraphicsCommandList* cmdList, PipelineService* psoService, RenderTargetType type) {
	BaseScene::Draw(cmdList, psoService, type);

	title_->Draw(cmdList);
}