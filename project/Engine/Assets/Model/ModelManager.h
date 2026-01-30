#pragma once

/* engine */
#include <Engine/Assets/Animation/AnimationStruct.h>
#include <Engine/Assets/Model/ModelData.h>
#include <Engine/Foundation/Math/Quaternion.h>
#include <Engine/Foundation/Utility/Func/MyFunc.h>
#include <Engine/Graphics/Context/GraphicsGroup.h>

/* Assimp */
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

/* DX12 */
#include <d3d12.h>
#include <wrl.h>

/* STL */
#include <condition_variable>
#include <functional>
#include <future>
#include <memory>
#include <mutex>
#include <queue>
#include <string>
#include <thread>
#include <unordered_map>

/*-----------------------------------------------------------------------------------------
 * ModelManager
 * - モデル管理クラス
 * - 3Dモデルファイルの非同期ロード、GPUリソースの生成、ロード済みデータの管理を担当
 *---------------------------------------------------------------------------------------*/
class ModelManager {
public:
	/**
	 * \brief インスタンスを取得
	 * \return インスタンス
	 */
	static ModelManager* GetInstance();

	/**
	 * \brief 初期化
	 */
	static void Initialize();

	/**
	 * \brief 終了処理
	 */
	static void Finalize();

	friend struct std::default_delete<ModelManager>;

	/**
	 * \brief 非同期でモデルをロードする (ワーカースレッド1本で順番に処理)
	 * \param fileName モデルのファイル名(例: "suzanne.obj")
	 * \return ロード完了時の ModelData* を返す future
	 *         （※ ModelData はコピー禁止のためポインタで返す）
	 */
	static std::future<ModelData*> LoadModel(const std::string& fileName);

	/**
	 * \brief ロードが完了したモデルの GPUリソース作成をまとめて行う
	 *        (フレーム更新時などメインスレッドで呼ぶ想定)
	 */
	void ProcessLoadingTasks();

	/**
	 * \brief ロード済みのモデルを取得
	 *        （ロード中または未ロードの場合はダミーを返す）
	 */
	ModelData& GetModelData(const std::string& fileName);

	/**
	 * \brief ファイル名からメッシュデータを取得
	 */
	MeshResource& GetMeshResource(const std::string& fileName);

	/**
	 * \brief モデルがロード済みか判定
	 */
	bool IsModelLoaded(const std::string& fileName) const;

	/**
	 * \brief ロード完了時に呼ばれるコールバックを設定する
	 */
	void SetOnModelLoadedCallback(std::function<void(const std::string&)> callback);

	/**
	 * \brief サンプル: 複数モデルを一括でロード
	 */
	static void StartUpLoad();

	/**
	 * \brief ロード済みモデル名の一覧を取得
	 */
	std::vector<std::string> GetLoadedModelNames() const;

private:
	ModelManager();
	~ModelManager();

	/// ファイル読み込み → CPU側 ModelData を構築（Assimp使用）
	ModelData LoadModelFile(const std::string& directoryPath, const std::string& fileNameWithExt);

	/// 頂点/インデックスバッファを GPU に作成して ModelData に登録
	void CreateGpuResources(const std::string& fileName, ModelData& model);

	/// メッシュ読み込み
	void LoadMesh(const aiMesh* mesh, ModelData& modelData);

	/// マテリアル読み込み
	void LoadMaterial(const aiScene* scene, const aiMesh* mesh, ModelData& modelData);

	/// UV変換情報読み込み
	void LoadUVTransform(const aiMaterial* material, MaterialData& outMaterial);

	/// スキニングデータ読み込み（未使用）
	void LoadSkinData(const aiMesh* mesh, ModelData& modelData);

	/// アニメーション評価
	CalyxMath::Vector3    Evaluate(const AnimationCurve<CalyxMath::Vector3>& curve, float time);
	CalyxMath::Quaternion Evaluate(const AnimationCurve<CalyxMath::Quaternion>& curve, float time);

private:
	//===================================================================
	//                    private member variables
	//===================================================================*/

	static std::unique_ptr<ModelManager> instance_;      //< シングルトンインスタンス
	static const std::string             directoryPath_; //< モデルディレクトリパス

	std::unordered_map<std::string, std::unique_ptr<ModelData>> modelDatas_;
	mutable std::mutex                        modelDataMutex_; //< モデルデータ用ミューテックス

	std::thread             workerThread_;       //< ワーカースレッド
	bool                    stopWorker_ = false; //< スレッド停止フラグ
	std::mutex              taskQueueMutex_;     //< タスクキュー用ミューテックス
	std::condition_variable taskQueueCv_;        //< タスクキュー用条件変数

	//-------------------------------
	// リクエスト構造体
	//-------------------------------
	struct LoadRequest {
		std::string fileName;              //< ファイル名
		std::promise<ModelData*> promise;  //< CPUロード完了後 future に値を返す
	};

	std::queue<LoadRequest> requestQueue_; //< リクエスト待ち行列

	//-------------------------------
	// GPUリソース化待ちタスク
	//-------------------------------
	struct LoadingTask {
		std::string                 fileName; //< ファイル名
		std::unique_ptr<ModelData> model;     //< move-only
	};

	std::vector<LoadingTask> pendingTasks_;
	std::mutex               pendingTasksMutex_;

	std::function<void(const std::string&)> onModelLoadedCallback_; //< ロード完了コールバック

	// ------------------------------------------------------
	// ワーカースレッド本体処理（CPUロード）
	// ------------------------------------------------------
	void WorkerMain();
};