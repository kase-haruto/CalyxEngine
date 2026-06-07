#include "AssetManager.h"
#include <Engine/Foundation/Audio/Audio.h>

/////////////////////////////////////////////////////////////////////////////////////////
//		インスタンス取得（シングルトン）
/////////////////////////////////////////////////////////////////////////////////////////
CalyxEngine::AssetManager* CalyxEngine::AssetManager::GetInstance()  {
	// 静的ローカル変数によるスレッドセーフな唯一のインスタンス生成
	static AssetManager instance;
	return &instance;
}

/////////////////////////////////////////////////////////////////////////////////////////
//		初期化
/////////////////////////////////////////////////////////////////////////////////////////
void CalyxEngine::AssetManager::Initialize(ImGuiManager* imgui) {
	// --- モデルマネージャーの生成と初期化 ---
	modelManager_ = std::make_unique<ModelManager>();
	modelManager_->Initialize();
	// デフォルトで読み込むべき初期モデルや共通形状のロード処理を開始
	modelManager_->StartUpLoad();

	// --- テクスチャマネージャーの生成と初期化 ---
	textureManager_ = std::make_unique<TextureManager>();
	// ImGuiでのプレビュー表示用にImGuiManagerポインタを渡す
	textureManager_->Initialize(imgui);
	// 共通アイコンなどの初期テクスチャリソースのロードを開始
	textureManager_->StartUpLoad();

	// --- データアセットマネージャーの生成 ---
	dataAssetManager_ = std::make_unique<DataAssetManager>();
	
	// --- オーディオエンジンの初期化 ---
	Audio::Initialize();
}

/////////////////////////////////////////////////////////////////////////////////////////
//		終了処理
/////////////////////////////////////////////////////////////////////////////////////////
void CalyxEngine::AssetManager::Finalize() {
	// 各スマートポインタのリセットを行い、マネージャーのデストラクタを呼び出す
	modelManager_.reset();
	textureManager_.reset();
	dataAssetManager_.reset();
	
	// オーディオエンジンの解放
	Audio::Finalize();
}
