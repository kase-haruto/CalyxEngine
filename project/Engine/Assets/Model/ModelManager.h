#pragma once

/* engine */
#include <Engine/Assets/Animation/AnimationStruct.h>
#include <Engine/Assets/Model/ModelData.h>
#include <Engine/Foundation/Export/CalyxAPI.h>
#include <Engine/Foundation/Math/Quaternion.h>
#include <Engine/Foundation/Utility/Func/MyFunc.h>

/* Assimp */
#include <assimp/scene.h>

/* STL */
#include <condition_variable>
#include <filesystem>
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

	ModelManager();
	~ModelManager();
	 
	/**
	 * \brief 初期化
	 */
	void Initialize();

	friend struct std::default_delete<ModelManager>;

	/**
	 * \brief 非同期でモデルをロードする (ワーカースレッド1本で順番に処理)
	 * \param fileName モデルのファイル名(例: "suzanne.obj")
	 * \return ロード完了時の ModelData* を返す future
	 *         （※ ModelData はコピー禁止のためポインタで返す）
	 */
	std::future<ModelData*> LoadModel(const std::string& fileName);

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
	void StartUpLoad();

	/**
	 * \brief 現在プロジェクトの AssetRoot を基準にモデル一覧を再読み込みする。
	 * \details Editor 起動後に .calyxproj が選択され、AssetRoot が変わった場合に使用する。
	 */
	CALYX_API void RefreshProjectAssets();

	/**
	 * \brief ロード済みモデル名の一覧を取得
	 */
	std::vector<std::string> GetLoadedModelNames() const;

private:
	/**
	 * \brief ディレクトリ内の全モデルファイルをロードするサンプル
	 */
	void LoadAllModels();

	/**
	 * \brief 現在のプロジェクトからモデルディレクトリを取得
	 */
	std::filesystem::path GetModelDirectoryPath() const;

	/// ファイル読み込み → CPU側 ModelData を構築（Assimp使用）
	ModelData LoadModelFile(const std::string& directoryPath, const std::string& fileNameWithExt);

	/// 頂点/インデックスバッファを GPU に作成して ModelData に登録
	void CreateGpuResources(const std::string& fileName, ModelData& model);

	/// メッシュ読み込み
	void LoadMesh(const aiMesh* mesh, ModelData& modelData);

	/// マテリアル読み込み
	void LoadMaterials(const aiScene* scene, const std::filesystem::path& modelDirectory, ModelData& modelData);

	/// UV変換情報読み込み
	void LoadUVTransform(const aiMaterial* material, MaterialData& outMaterial);

	/// スキニングデータ読み込み（未使用）
	void LoadSkinData(const aiMesh* mesh, ModelData& modelData);

	/// アニメーション評価
	CalyxEngine::Vector3    Evaluate(const AnimationCurve<CalyxEngine::Vector3>& curve, float time);
	CalyxEngine::Quaternion Evaluate(const AnimationCurve<CalyxEngine::Quaternion>& curve, float time);

private:
	//===================================================================
	//                    private member variables
	//===================================================================*/

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
