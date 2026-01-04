#pragma once

#include <Engine/Assets/Animation/AnimationStruct.h>
#include <Engine/Assets/Model/ModelData.h>
#include <Engine/Graphics/Context/GraphicsGroup.h>

#include <Engine/Foundation/Utility/Func/MyFunc.h>
#include <Engine/Foundation/Math/Quaternion.h>

// Assimp
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

// DX12
#include <d3d12.h>
#include <wrl.h>

// STL
#include <condition_variable>
#include <functional>
#include <future>
#include <memory>
#include <mutex>
#include <queue>
#include <string>
#include <thread>
#include <unordered_map>

class ModelManager {
public:
	static ModelManager* GetInstance();
	static void Initialize();
	static void Finalize();

	/// <summary>
	/// 非同期でモデルをロードする (ワーカースレッド1本で順番に処理)
	/// </summary>
	/// <param name="fileName">モデルのファイル名(例: "suzanne.obj")</param>
	/// <returns>ロード完了時の ModelData を指す future</returns>
	static std::future<ModelData> LoadModel(const std::string& fileName);

	/// <summary>
	/// ロードが完了したモデルの GPUリソース作成をまとめて行う
	/// (フレーム更新時などメインスレッドで呼ぶ想定)
	/// </summary>
	void ProcessLoadingTasks();

	/// <summary>
	/// ロード済みのモデルを取得(非同期ロード中の場合は nullptr の可能性もある)
	/// </summary>
	ModelData& GetModelData(const std::string& fileName);

	//ロード済みのモデルをを判定
	bool IsModelLoaded(const std::string& fileName) const;

	/// <summary>
	/// ロード完了時に呼ばれるコールバックを設定する
	/// </summary>
	void SetOnModelLoadedCallback(std::function<void(const std::string&)> callback);

	/// <summary>
	/// サンプル: 複数モデルを一括でロード
	/// </summary>
	static void StartUpLoad();

	/// <summary>
	/// ロード済みモデルのファイル名一覧を取得
	/// </summary>
	std::vector<std::string> GetLoadedModelNames() const;

private:
	ModelManager();
	~ModelManager();

	/// ファイルを読み込み→CPU側でModelDataを構築する (Assimp使用)
	ModelData LoadModelFile(const std::string& directoryPath, const std::string& fileNameWithExt);

	/// 頂点/インデックスバッファを GPU上に作成し、ModelData に登録
	void CreateGpuResources(const std::string& fileName, ModelData model);

	// メッシュやマテリアルなどの細かい読み込み
	void LoadMesh(const aiMesh* mesh, ModelData& modelData);
	void LoadMaterial(const aiScene* scene, const aiMesh* mesh, ModelData& modelData);
	void LoadUVTransform(const aiMaterial* material, MaterialData& outMaterial);
	void LoadSkinData(const aiMesh* mesh, ModelData& modelData);
	// アニメーション評価関連
	Vector3 Evaluate(const AnimationCurve<Vector3>& curve, float time);
	Quaternion Evaluate(const AnimationCurve<Quaternion>& curve, float time);

private:
	// シングルトン
	static ModelManager* instance_;
	static const std::string directoryPath_;

	// ------------------------------------------------------
	// モデルデータマップ (ファイル名 -> ModelData)
	// ------------------------------------------------------
	std::unordered_map<std::string, ModelData> modelDatas_;
	mutable std::mutex modelDataMutex_;

	// ------------------------------------------------------
	// ワーカースレッド (1本)
	// ------------------------------------------------------
	std::thread workerThread_;
	bool stopWorker_ = false;
	std::mutex taskQueueMutex_;
	std::condition_variable taskQueueCv_;

	// ロードリクエスト
	struct LoadRequest {
		std::string fileName;
		std::promise<ModelData> promise;
	};
	std::queue<LoadRequest> requestQueue_; // リクエスト待ち行列

	// ------------------------------------------------------
	// CPUロード完了後のタスク (GPUリソース化待ち)
	// ------------------------------------------------------
	struct LoadingTask {
		std::string fileName;
		ModelData modelData;
	};
	std::vector<LoadingTask> pendingTasks_;
	std::mutex pendingTasksMutex_;

	// ロード完了コールバック
	std::function<void(const std::string&)> onModelLoadedCallback_;

	// ------------------------------------------------------
	// ワーカースレッド本体処理
	// ------------------------------------------------------
	void WorkerMain();
};
