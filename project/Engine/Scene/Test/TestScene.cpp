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

	//SceneSerializer::Load(*sceneContext_, filePath);

	sceneContext_->Initialize();
	sceneContext_->SetSceneName("TestScene");


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
void TestScene::Update(){

	CameraManager::Update();

	skyBox_->Update();

	//衝突判定
	CollisionManager::GetInstance()->UpdateCollisionAllCollider();

	sceneContext_->Update();
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



