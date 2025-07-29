/////////////////////////////////////////////////////////////////////////////////////////
//	include
/////////////////////////////////////////////////////////////////////////////////////////

// scene
#include <Engine/Scene/Test/TestScene.h>

// engine
#include <Engine/Application/Input/Input.h>
#include <Engine/Collision/CollisionManager.h>
#include <Engine/Graphics/Camera/Manager/CameraManager.h>
#include <Engine/Foundation/Utility/Func/MyFunc.h>
#include <Engine/Graphics/Pipeline/Service/PipelineService.h>
#include <Engine/Scene/Serializer/SceneSerializer.h>
// lib

/////////////////////////////////////////////////////////////////////////////////////////
//	コンストラクタ/デストラクタ
/////////////////////////////////////////////////////////////////////////////////////////
TestScene::TestScene(){
	// シーン名を設定
	BaseScene::SetSceneName("TestScene");

}

/////////////////////////////////////////////////////////////////////////////////////////
//	アセットのロード
/////////////////////////////////////////////////////////////////////////////////////////
void TestScene::LoadAssets(){}

/////////////////////////////////////////////////////////////////////////////////////////
//	初期化処理
/////////////////////////////////////////////////////////////////////////////////////////
void TestScene::Initialize(){
	sceneContext_->Initialize();

	sceneContext_->SetSceneName("TestScene");

	BaseScene::Initialize();

	SceneSerializer::Load(*sceneContext_, "Resources/Assets/Scenes/TestScene.scene");

	LoadAssets();

	CameraManager::GetInstance()->SetType(CameraType::Type_Debug);
	auto* cam = CameraManager::GetInstance()->GetCamera3d();
	cam->Initialize();
	//=========================
	// グラフィック関連
	//=========================

	skyBox_ = std::make_unique<SkyBox>("sky.dds", "skyBox");
	skyBox_->Initialize();
}

/////////////////////////////////////////////////////////////////////////////////////////
//	更新処理
/////////////////////////////////////////////////////////////////////////////////////////
void TestScene::Update(float dt){

	ImGui::Begin("Play Control");

	switch (playSession_.GetMode()){
		case EngineMode::Editor:
			if (ImGui::Button("▶ Play")) playSession_.Enter();
			break;

		case EngineMode::Playing:
			if (ImGui::Button("■ Stop")) playSession_.Exit();
			ImGui::SameLine();
			if (ImGui::Button("⏸ Pause")) playSession_.TogglePause();
			ImGui::SameLine();
			if (ImGui::Button("🔁 Restart")) playSession_.Restart();
			break;

		case EngineMode::Paused:
			if (ImGui::Button("■ Stop")) playSession_.Exit();
			ImGui::SameLine();
			if (ImGui::Button("▶ Resume")) playSession_.TogglePause();
			ImGui::SameLine();
			if (ImGui::Button("⏭ Step")) playSession_.StepOnce();
			ImGui::SameLine();
			if (ImGui::Button("🔁 Restart")) playSession_.Restart();
			break;
	}

	ImGui::End();

	playSession_.Update(dt);

	CameraManager::Update(dt);

	//衝突判定
	CollisionManager::GetInstance()->UpdateCollisionAllCollider();
}

void TestScene::Draw(ID3D12GraphicsCommandList* cmdList, PipelineService* psoService, RenderTargetType type){

	//========================================================//
	//	spriteの登録
	//========================================================//
	// 
	// 
	//シーン上のオブジェクトの描画
	BaseScene::Draw(cmdList, psoService, type);

}

void TestScene::CleanUp(){
	// 3Dオブジェクトの描画を終了
	sceneContext_->GetObjectLibrary()->Clear();
	CollisionManager::GetInstance()->ClearColliders();
}



