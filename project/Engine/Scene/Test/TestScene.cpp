/////////////////////////////////////////////////////////////////////////////////////////
//	include
/////////////////////////////////////////////////////////////////////////////////////////

// scene
#include <Engine/Scene/Test/TestScene.h>
#include <Engine/Scene/Utirity/SceneUtility.h>

// engine
#include <Engine/Application/Input/Input.h>
#include <Engine/Collision/CollisionManager.h>
#include <Engine/Graphics/Camera/Manager/CameraManager.h>
#include <Engine/Foundation/Utility/Func/MyFunc.h>
#include <Engine/Graphics/Pipeline/Service/PipelineService.h>

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


	LoadAssets();

	CameraManager::GetInstance()->SetType(CameraType::Type_Debug);
	//=========================
	// グラフィック関連
	//=========================

	//skyBox_ = std::make_unique<SkyBox>("sky.dds", "skyBox");
	//skyBox_->Initialize();

	//=========================
	// オブジェクト生成
	//=========================

	testObjects_.resize(80);
	int count = 0;

	const int gridX = 5;
	const int gridY = 4;
	const int gridZ = 4;
	const float spacing = 2.0f;  // 間隔（単位：ユニット）

	for (int z = 0; z < gridZ; ++z){
		for (int y = 0; y < gridY; ++y){
			for (int x = 0; x < gridX; ++x){
				if (count >= static_cast< int >(testObjects_.size())) break;

				auto* obj = sceneContext_->GetObjectLibrary()->CreateAndAddObject<BaseGameObject>("debugCube.obj", "cube");
				obj->GetCollider()->SetIsDrawCollider(false);
				testObjects_[count++] = obj;
				// 原点中心に配置したい場合は中央に寄せる
				Vector3 position = {
					(x - gridX / 2.0f) * spacing,
					(y - gridY / 2.0f) * spacing,
					(z - gridZ / 2.0f) * spacing
				};

				obj->SetTranslate(position);
			}
		}
	}

}

/////////////////////////////////////////////////////////////////////////////////////////
//	更新処理
/////////////////////////////////////////////////////////////////////////////////////////
void TestScene::Update(){

	CameraManager::Update();

	//	skyBox_->Update();
	for (auto& obj : testObjects_){
		obj->Update();
	}

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



