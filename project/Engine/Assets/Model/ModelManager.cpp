#include "ModelManager.h"

#include <Engine/Graphics/Buffer/DxVertexBuffer.h>
#include <Engine/Graphics/Buffer/DxIndexBuffer.h>
#include <Engine/Graphics/Pipeline/PipelineDesc/Input/VertexLayout.h>

// static 変数初期化
ModelManager* ModelManager::instance_ = nullptr;
const std::string ModelManager::directoryPath_ = "Resources/models";

ModelManager::ModelManager(){
	// スレッドを起動
	workerThread_ = std::thread(&ModelManager::WorkerMain, this);
}

ModelManager::~ModelManager(){
	// ワーカー終了指示
	{
		std::lock_guard<std::mutex> lock(taskQueueMutex_);
		stopWorker_ = true;
	}
	taskQueueCv_.notify_all();

	if (workerThread_.joinable()){
		workerThread_.join();
	}
}

ModelManager* ModelManager::GetInstance(){
	if (!instance_){
		instance_ = new ModelManager();
	}
	return instance_;
}

void ModelManager::Initialize(){
	GetInstance();
}

void ModelManager::Finalize(){
	if (instance_){
		delete instance_;
		instance_ = nullptr;
	}
}

//----------------------------------------------------------------------------
// 非同期ロード開始
//----------------------------------------------------------------------------
std::future<ModelData> ModelManager::LoadModel(const std::string& fileName){
	// 既にロード済みかどうかチェック
	{
		std::lock_guard<std::mutex> lock(instance_->modelDataMutex_);
		auto it = instance_->modelDatas_.find(fileName);
		if (it != instance_->modelDatas_.end()){
			// 既にロード済みなら、即座に value を設定した future を返す
			std::promise<ModelData> promise;
			promise.set_value(it->second);
			return promise.get_future();
		}
	}

	// キューに新しいリクエストを積む
	LoadRequest request;
	request.fileName = fileName;
	std::future<ModelData> fut = request.promise.get_future();

	{
		std::lock_guard<std::mutex> lock(instance_->taskQueueMutex_);
		instance_->requestQueue_.push(std::move(request));
	}
	instance_->taskQueueCv_.notify_one();

	return fut;
}

//----------------------------------------------------------------------------
// ワーカースレッドのループ処理
//----------------------------------------------------------------------------
void ModelManager::WorkerMain(){
	while (true){
		LoadRequest currentRequest;
		{
			// リクエスト待ち
			std::unique_lock<std::mutex> lock(taskQueueMutex_);
			taskQueueCv_.wait(lock, [this]{
				return stopWorker_ || !requestQueue_.empty();
							  });
			if (stopWorker_ && requestQueue_.empty()){
				// 終了指示
				return;
			}
			// キュー先頭を取り出し
			currentRequest = std::move(requestQueue_.front());
			requestQueue_.pop();
		}

		// ファイルを読み込み
		ModelData newModel = LoadModelFile(directoryPath_, currentRequest.fileName);

		// (B) GPUリソース作成はメインスレッドで行うため、一旦 pendingTasks_ に格納
		{
			std::lock_guard<std::mutex> lock(pendingTasksMutex_);
			pendingTasks_.push_back({currentRequest.fileName, newModel});
		}

		// CPUロード完了を promise で通知
		currentRequest.promise.set_value(newModel);
	}
}

//----------------------------------------------------------------------------
// (メインスレッド) CPUロードが終わったタスクを取り出して GPUリソースを作成
//----------------------------------------------------------------------------
void ModelManager::ProcessLoadingTasks(){
	std::vector<LoadingTask> tasks;
	{
		std::lock_guard<std::mutex> lock(pendingTasksMutex_);
		tasks.swap(pendingTasks_);
	}

	for (auto& t : tasks){
		// GPUリソースを生成
		CreateGpuResources(t.fileName, t.modelData);

		// modelDatas_ に登録
		{
			std::lock_guard<std::mutex> lock(modelDataMutex_);
			modelDatas_[t.fileName] = t.modelData;
		}

		// コールバック呼び出し(任意)
		if (onModelLoadedCallback_){
			onModelLoadedCallback_(t.fileName);
		}
	}
}

//----------------------------------------------------------------------------
// ロード済みモデルを取得（まだロード中なら nullptr）
//----------------------------------------------------------------------------
ModelData& ModelManager::GetModelData(const std::string& fileName){
	std::lock_guard<std::mutex> lock(modelDataMutex_);
	auto it = modelDatas_.find(fileName);
	if (it != modelDatas_.end()){
		return it->second;
	}

	// ロード中のモデルは plane を返す
	const std::string defaltModel = "plane.obj";
	auto lodingModel = modelDatas_.find(defaltModel);
	return lodingModel->second;
}

bool ModelManager::IsModelLoaded(const std::string& fileName) const{
	std::lock_guard<std::mutex> lock(modelDataMutex_);
	return modelDatas_.find(fileName) != modelDatas_.end();
}

//----------------------------------------------------------------------------
// ロード完了時のコールバック設定
//----------------------------------------------------------------------------
void ModelManager::SetOnModelLoadedCallback(std::function<void(const std::string&)> callback){
	onModelLoadedCallback_ = callback;
}

//----------------------------------------------------------------------------
// 複数ファイルをまとめてロード (サンプル)
//----------------------------------------------------------------------------
void ModelManager::StartUpLoad(){
	LoadModel("run.gltf");
	LoadModel("idle.gltf");
	LoadModel("player.gltf");
	LoadModel("ghost.obj");
	LoadModel("debugCube.obj");
	LoadModel("cylinder.obj");
	LoadModel("cone.obj");
	LoadModel("torus.obj");
	LoadModel("ground.obj");
	LoadModel("plane.obj");
}

//----------------------------------------------------------------------------
// ロード済みモデル名の一覧を返す
//----------------------------------------------------------------------------
std::vector<std::string> ModelManager::GetLoadedModelNames() const{
	std::lock_guard<std::mutex> lock(modelDataMutex_);
	std::vector<std::string> result;
	for (auto& kv : modelDatas_){
		result.push_back(kv.first);
	}
	return result;
}

//=============================================================================
//  
//=============================================================================
ModelData ModelManager::LoadModelFile(const std::string& directoryPath, const std::string& fileNameWithExt){
	Assimp::Importer importer;

	// パスを組み立て
	std::string filePath = directoryPath + "/"
		+ fileNameWithExt.substr(0, fileNameWithExt.find_last_of('.')) + "/"
		+ fileNameWithExt;

	const aiScene* scene = importer.ReadFile(
		filePath.c_str(),
		aiProcess_Triangulate |
		aiProcess_FlipUVs |
		aiProcess_CalcTangentSpace
	);
	if (!scene){
		throw std::runtime_error("Assimp failed to load: " + filePath);
	}
	if (!scene->HasMeshes()){
		throw std::runtime_error("No meshes found in file: " + filePath);
	}

	ModelData modelData;

	// メッシュデータを格納
	for (unsigned int meshIndex = 0; meshIndex < scene->mNumMeshes; ++meshIndex){
		const aiMesh* mesh = scene->mMeshes[meshIndex];
		LoadMesh(mesh, modelData);

		// ボーンごとの影響を集約
		for (uint32_t boneIndex = 0; boneIndex < mesh->mNumBones; ++boneIndex){

			aiBone* bone = mesh->mBones[boneIndex];
			std::string jointName = bone->mName.C_Str();
			JointWeightData& jointWeightData = modelData.skinClusterData[jointName];

			aiMatrix4x4 bindPoseMatrixAssimp = bone->mOffsetMatrix.Inverse();
			aiVector3D scale, translate;
			aiQuaternion rotate;
			bindPoseMatrixAssimp.Decompose(scale, rotate, translate);

			Matrix4x4 bindPoseMatrix =
				MakeAffineMatrix({scale.x,scale.y,scale.z}, {rotate.x,-rotate.y,-rotate.z,rotate.w}, {-translate.x,translate.y,translate.z});
			jointWeightData.inverseBindPoseMatrix = Matrix4x4::Inverse(bindPoseMatrix);

			for (uint32_t weightIndex = 0; weightIndex < bone->mNumWeights; ++weightIndex){

				jointWeightData.vertexWeights.push_back({bone->mWeights[weightIndex].mWeight, bone->mWeights[weightIndex].mVertexId});
			}
		}


		LoadMaterial(scene, mesh, modelData);
	}

	// アニメーション(サンプル)
	if (scene->HasAnimations()){
		aiAnimation* aiAnim = scene->mAnimations[0];
		Animation animation;
		float ticksPerSecond = ( float ) (aiAnim->mTicksPerSecond != 0 ? aiAnim->mTicksPerSecond : 25.0f);
		animation.duration = ( float ) (aiAnim->mDuration / ticksPerSecond);

		for (unsigned int channelIdx = 0; channelIdx < aiAnim->mNumChannels; ++channelIdx){
			aiNodeAnim* nodeAnim = aiAnim->mChannels[channelIdx];
			NodeAnimation nodeAnimation;

			// Translation Key
			for (uint32_t keyIndex = 0; keyIndex < nodeAnim->mNumPositionKeys; ++keyIndex){
				aiVectorKey& keyAssimp = nodeAnim->mPositionKeys[keyIndex];
				KeyframeVector3 keyframe;
				keyframe.time = static_cast< float >(keyAssimp.mTime / aiAnim->mTicksPerSecond);
				keyframe.value = {-keyAssimp.mValue.x, keyAssimp.mValue.y, keyAssimp.mValue.z}; // 右手→左手
				nodeAnimation.translate.keyframes.push_back(keyframe);
			}

			// Rotation Key
			for (uint32_t keyIndex = 0; keyIndex < nodeAnim->mNumRotationKeys; ++keyIndex){
				aiQuatKey& keyAssimp = nodeAnim->mRotationKeys[keyIndex];
				KeyframeQuaternion keyframe;
				keyframe.time = static_cast< float >(keyAssimp.mTime / aiAnim->mTicksPerSecond);
				keyframe.value = {keyAssimp.mValue.x, -keyAssimp.mValue.y, -keyAssimp.mValue.z, keyAssimp.mValue.w}; // 右手→左手
				nodeAnimation.rotate.keyframes.push_back(keyframe);
			}

			// Scaling Key
			for (uint32_t keyIndex = 0; keyIndex < nodeAnim->mNumScalingKeys; ++keyIndex){
				aiVectorKey& keyAssimp = nodeAnim->mScalingKeys[keyIndex];
				KeyframeVector3 keyframe;
				keyframe.time = static_cast< float >(keyAssimp.mTime / aiAnim->mTicksPerSecond);
				keyframe.value = {keyAssimp.mValue.x, keyAssimp.mValue.y, keyAssimp.mValue.z}; // スケールはそのまま
				nodeAnimation.scale.keyframes.push_back(keyframe);
			}
			std::string nodeName(nodeAnim->mNodeName.C_Str());
			animation.nodeAnimations[nodeName] = nodeAnimation;
		}
		modelData.animation = animation;
	}

	// スケルトン構築
	Node rootNode = ConvertAssimpNode(scene->mRootNode);
	modelData.skeleton = CreateSkeleton(rootNode);

	return modelData;
}

//----------------------------------------------------------------------------
// GPUリソース作成 (ProcessLoadingTasksから呼ばれる)
//----------------------------------------------------------------------------
void ModelManager::CreateGpuResources([[maybe_unused]] const std::string& fileName, ModelData model){
	auto device = GraphicsGroup::GetInstance()->GetDevice();

	DxVertexBuffer<VertexPosUvN> new_vertexBuffer;
	new_vertexBuffer.Initialize(device);
	DxIndexBuffer<uint32_t> new_indexBuffer;
	new_indexBuffer.Initialize(device);

	model.vertexBuffer = new_vertexBuffer;
	model.indexBuffer = new_indexBuffer;
}

//----------------------------------------------------------------------------
// メッシュ読み込み
//----------------------------------------------------------------------------
void ModelManager::LoadMesh(const aiMesh* mesh, ModelData& modelData){
	uint32_t baseVertex = static_cast< uint32_t >(modelData.meshData.vertices.size());

	// 初期AABBを極端な値に
	Vector3 minPos = {FLT_MAX, FLT_MAX, FLT_MAX};
	Vector3 maxPos = {-FLT_MAX, -FLT_MAX, -FLT_MAX};

	for (unsigned int i = 0; i < mesh->mNumVertices; ++i){
		VertexPosUvN vertex {};
		vertex.position = {-mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z, 1.0f};
		if (mesh->HasNormals()){
			vertex.normal = {-mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z};
		}
		if (mesh->HasTextureCoords(0)){
			vertex.texcoord.x = mesh->mTextureCoords[0][i].x;
			vertex.texcoord.y = mesh->mTextureCoords[0][i].y;
		}
		modelData.meshData.vertices.push_back(vertex);

		// AABB更新用の min/max 反映
		Vector3 pos = {vertex.position.x, vertex.position.y, vertex.position.z};
		minPos = Vector3::Min(minPos, pos);
		maxPos = Vector3::Max(maxPos, pos);
	}

	for (unsigned int i = 0; i < mesh->mNumFaces; ++i){
		const aiFace& face = mesh->mFaces[i];
		modelData.meshData.indices.push_back(baseVertex + face.mIndices[0]);
		modelData.meshData.indices.push_back(baseVertex + face.mIndices[2]);
		modelData.meshData.indices.push_back(baseVertex + face.mIndices[1]);
	}

	// ローカルAABBを格納
	if (modelData.localAABB.min_ == Vector3 {} && modelData.localAABB.max_ == Vector3 {}){
		modelData.localAABB.Initialize(minPos, maxPos);
	} else{
		// モデル全体の AABB を統合（複数メッシュ時）
		Vector3 mergedMin = Vector3::Min(modelData.localAABB.min_, minPos);
		Vector3 mergedMax = Vector3::Max(modelData.localAABB.max_, maxPos);
		modelData.localAABB.Initialize(mergedMin, mergedMax);
	}
}

//----------------------------------------------------------------------------
// マテリアル読み込み
//----------------------------------------------------------------------------
void ModelManager::LoadMaterial(const aiScene* scene, const aiMesh* mesh, ModelData& modelData){
	if (!scene->HasMaterials()){
		modelData.meshData.material.textureFilePath = "white1x1.png";
		return;
	}
	const aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];
	if (!material){
		modelData.meshData.material.textureFilePath = "white1x1.png";
		return;
	}

	aiString texPath;
	if (material->GetTexture(aiTextureType_DIFFUSE, 0, &texPath) == AI_SUCCESS){
		modelData.meshData.material.textureFilePath = texPath.C_Str();
	} else{
		modelData.meshData.material.textureFilePath = "white1x1.png";
	}

	LoadUVTransform(material, modelData.meshData.material);
}

//----------------------------------------------------------------------------
// UV変換情報
//----------------------------------------------------------------------------
void ModelManager::LoadUVTransform(const aiMaterial* material, MaterialData& outMaterial){
	aiUVTransform transformData;
	if (material->Get(AI_MATKEY_UVTRANSFORM(aiTextureType_DIFFUSE, 0), transformData) == AI_SUCCESS){
		outMaterial.uv_offset = {transformData.mTranslation.x, transformData.mTranslation.y, 0.0f};
		outMaterial.uv_scale = {transformData.mScaling.x, transformData.mScaling.y, 1.0f};
	} else{
		outMaterial.uv_offset = {0, 0, 0};
		outMaterial.uv_scale = {1, 1, 1};
	}
}

void ModelManager::LoadSkinData([[maybe_unused]] const aiMesh* mesh, [[maybe_unused]] ModelData& modelData){

}

//----------------------------------------------------------------------------
// アニメーション評価サンプル
//----------------------------------------------------------------------------
Vector3 ModelManager::Evaluate(const AnimationCurve<Vector3>& curve, float time){
	const auto& keyframes = curve.keyframes;
	if (keyframes.empty()){
		return {0,0,0};
	}
	if (time <= keyframes.front().time){
		return keyframes.front().value;
	}
	if (time >= keyframes.back().time){
		return keyframes.back().value;
	}
	for (int i = 0; i < ( int ) keyframes.size() - 1; ++i){
		float t0 = keyframes[i].time;
		float t1 = keyframes[i + 1].time;
		if (time >= t0 && time <= t1){
			float localT = (time - t0) / (t1 - t0);
			return Vector3::Lerp(keyframes[i].value, keyframes[i + 1].value, localT);
		}
	}
	return keyframes.back().value;
}

Quaternion ModelManager::Evaluate(const AnimationCurve<Quaternion>& curve, float time){
	const auto& keyframes = curve.keyframes;
	if (keyframes.empty()){
		return {0,0,0,1};
	}
	if (time <= keyframes.front().time){
		return keyframes.front().value;
	}
	if (time >= keyframes.back().time){
		return keyframes.back().value;
	}
	for (int i = 0; i < ( int ) keyframes.size() - 1; ++i){
		float t0 = keyframes[i].time;
		float t1 = keyframes[i + 1].time;
		if (time >= t0 && time <= t1){
			float localT = (time - t0) / (t1 - t0);
			return Quaternion::Slerp(keyframes[i].value, keyframes[i + 1].value, localT);
		}
	}
	return keyframes.back().value;
}
