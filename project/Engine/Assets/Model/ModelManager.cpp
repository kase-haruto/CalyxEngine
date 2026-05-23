#include "ModelManager.h"

// engine
#include "Engine/Foundation/Math/MathUtil.h"
#include "Engine/Graphics/Context/GraphicsGroup.h"
#include <Engine/Graphics/Buffer/DxIndexBuffer.h>
#include <Engine/Graphics/Buffer/DxVertexBuffer.h>
#include <Engine/Graphics/Pipeline/PipelineDesc/Input/VertexLayout.h>
#include <Engine/Foundation/Utility/FileSystem/FileScanner.h>

// externals
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>

#include <filesystem>
#include <sstream>


ModelManager::ModelManager() {
	// スレッドを起動
	workerThread_ = std::thread(&ModelManager::WorkerMain, this);
}

ModelManager::~ModelManager() {
	// ワーカー終了指示
	{
		std::lock_guard<std::mutex> lock(taskQueueMutex_);
		stopWorker_ = true;
	}
	taskQueueCv_.notify_all();

	if(workerThread_.joinable()) {
		workerThread_.join();
	}
}

void ModelManager::Initialize() {
	LoadAllModels();
}


//----------------------------------------------------------------------------
// 非同期ロード開始
//----------------------------------------------------------------------------
std::future<ModelData*> ModelManager::LoadModel(const std::string& fileName) {
	{
		std::lock_guard<std::mutex> lock(modelDataMutex_);
		auto						it = modelDatas_.find(fileName);
		if(it != modelDatas_.end()) {
			std::promise<ModelData*> promise;
			promise.set_value(it->second.get());
			return promise.get_future();
		}
	}

	LoadRequest request;
	request.fileName			= fileName;
	std::future<ModelData*> fut = request.promise.get_future();

	{
		std::lock_guard<std::mutex> lock(taskQueueMutex_);
		requestQueue_.push(std::move(request));
	}
	taskQueueCv_.notify_one();

	return fut;
}

//----------------------------------------------------------------------------
// ワーカースレッドのループ処理
//----------------------------------------------------------------------------
void ModelManager::WorkerMain() {
	while(true) {
		LoadRequest currentRequest;

		{
			std::unique_lock<std::mutex> lock(taskQueueMutex_);
			taskQueueCv_.wait(lock, [this] { return stopWorker_ || !requestQueue_.empty(); });

			if(stopWorker_ && requestQueue_.empty()) {
				return;
			}

			currentRequest = std::move(requestQueue_.front());
			requestQueue_.pop();
		}

		// CPUロード
		ModelData model = LoadModelFile(directoryPath_, currentRequest.fileName);

		// unique_ptr に包む
		auto modelPtr = std::make_unique<ModelData>(std::move(model));

		// pending タスクに積む
		{
			std::lock_guard<std::mutex> lock(pendingTasksMutex_);
			pendingTasks_.push_back({currentRequest.fileName,
									 std::move(modelPtr)});
		}

		// future に *まだ nullptr を返す*（GPU リソース未作成のため）
		currentRequest.promise.set_value(nullptr);
	}
}

//----------------------------------------------------------------------------
// (メインスレッド) CPUロードが終わったタスクを取り出して GPUリソースを作成
//----------------------------------------------------------------------------
void ModelManager::ProcessLoadingTasks() {
	std::vector<LoadingTask> tasks;

	{
		std::lock_guard<std::mutex> lock(pendingTasksMutex_);
		tasks.swap(pendingTasks_);
	}

	for(auto& t : tasks) {
		// 必ず GPU リソースを作成
		CreateGpuResources(t.fileName, *t.model);

		// modelDatas_ に move で保存
		{
			std::lock_guard<std::mutex> lock(modelDataMutex_);
			modelDatas_[t.fileName] = std::move(t.model);
		}

		if(onModelLoadedCallback_) {
			onModelLoadedCallback_(t.fileName);
		}
	}
}

//----------------------------------------------------------------------------
// ロード済みモデルを取得（まだロード中なら nullptr）
//----------------------------------------------------------------------------
ModelData& ModelManager::GetModelData(const std::string& fileName) {
	std::lock_guard<std::mutex> lock(modelDataMutex_);

	auto it = modelDatas_.find(fileName);
	if(it != modelDatas_.end()) {
		return *(it->second);
	}

	const std::string defaultModel = "plane.obj";
	auto			  it2		   = modelDatas_.find(defaultModel);
	if(it2 != modelDatas_.end()) {
		return *(it2->second);
	}

	static ModelData dummy;
	return dummy;
}

MeshResource& ModelManager::GetMeshResource(const std::string& fileName) {
	std::lock_guard<std::mutex> lock(modelDataMutex_);

	if(auto it = modelDatas_.find(fileName); it != modelDatas_.end()) {
		return it->second->meshResource;
	}

	const std::string defaultModel = "plane.obj";
	if(auto it = modelDatas_.find(defaultModel); it != modelDatas_.end()) {
		return it->second->meshResource;
	}

	static ModelData dummy; // 未ロード時の安全なフォールバック
	return dummy.meshResource;
}

bool ModelManager::IsModelLoaded(const std::string& fileName) const {
	std::lock_guard<std::mutex> lock(modelDataMutex_);
	return modelDatas_.find(fileName) != modelDatas_.end();
}

//----------------------------------------------------------------------------
// ロード完了時のコールバック設定
//----------------------------------------------------------------------------
void ModelManager::SetOnModelLoadedCallback(std::function<void(const std::string&)> callback) { onModelLoadedCallback_ = callback; }

//----------------------------------------------------------------------------
// 複数ファイルをまとめてロード (サンプル)
//----------------------------------------------------------------------------
void ModelManager::StartUpLoad() {
	
}

//----------------------------------------------------------------------------
// ロード済みモデル名の一覧を返す
//----------------------------------------------------------------------------
std::vector<std::string> ModelManager::GetLoadedModelNames() const {
	std::lock_guard<std::mutex> lock(modelDataMutex_);
	std::vector<std::string>	result;
	for(auto& kv : modelDatas_) {
		result.push_back(kv.first);
	}
	return result;
}

//=============================================================================
//	ファイルを走破してすべてのモデルファイルをload
//=============================================================================
void ModelManager::LoadAllModels() {
	// ファイルを走破してモデルをロードする なければエラーをはく
	if(!CalyxEngine::FileScanner::EnsureDirectoryExists(directoryPath_)) {
		throw std::runtime_error("Model directory does not exist: " + directoryPath_);
	}
	
	// Assimでサポートされているモデル形式の拡張子
	const std::vector<std::string> supportedExtensions = {
		".obj",   // OBJ format
		".fbx",   // Autodesk FBX
		".gltf",  // glTF
	};
	
	// ディレクトリ内のすべてのファイルをスキャン
	auto allFiles = CalyxEngine::FileScanner::ScanFiles(directoryPath_, "");
	
	for(const auto& filePath : allFiles) {
		std::string ext = filePath.extension().string();
		
		// 拡張子を小文字に変換してチェック（比較用のみ）
		std::string extLower = ext;
		std::transform(extLower.begin(), extLower.end(), extLower.begin(), 
			[](unsigned char c) -> char { return static_cast<char>(std::tolower(c)); });
		
		// サポートされている拡張子か確認
		bool isSupported = false;
		for(const auto& supportedExt : supportedExtensions) {
			if(extLower == supportedExt) {
				isSupported = true;
				break;
			}
		}
	
		if(isSupported) {
			// ファイル名（拡張子含む）を取得してロード開始（元のファイル名をそのまま使用）
			std::string fileName = filePath.filename().string();
			LoadModel(fileName);
		}
	}
}

//=============================================================================
//
//=============================================================================ｔ
ModelData ModelManager::LoadModelFile(const std::string& directoryPath, const std::string& fileNameWithExt) {
	Assimp::Importer importer;

	// ファイル名から拡張子を除いた部分を取得
	std::string modelNameBase = fileNameWithExt.substr(0, fileNameWithExt.find_last_of('.'));
	
	// ディレクトリ名用に小文字に変換（サブディレクトリが小文字であるため）
	std::string modelNameLower = modelNameBase;
	std::transform(modelNameLower.begin(), modelNameLower.end(), modelNameLower.begin(), 
		[](unsigned char c) -> char { return static_cast<char>(std::tolower(c)); });

	// パスを組み立て
	std::string filePath = directoryPath + "/" + modelNameLower + "/" + fileNameWithExt;
	std::filesystem::path modelDirectory = std::filesystem::path(filePath).parent_path();

	const aiScene* scene = importer.ReadFile(
		filePath.c_str(),
		aiProcess_Triangulate |
			aiProcess_FlipUVs |
			aiProcess_CalcTangentSpace);
	if(!scene) {
		throw std::runtime_error("Assimp failed to load: " + filePath);
	}
	if(!scene->HasMeshes()) {
		throw std::runtime_error("No meshes found in file: " + filePath);
	}

	ModelData modelData;
	LoadMaterials(scene, modelDirectory, modelData);

	// メッシュデータを格納
	for(unsigned int meshIndex = 0; meshIndex < scene->mNumMeshes; ++meshIndex) {
		const aiMesh* mesh = scene->mMeshes[meshIndex];
		LoadMesh(mesh, modelData);

		// ボーンごとの影響を集約
		for(uint32_t boneIndex = 0; boneIndex < mesh->mNumBones; ++boneIndex) {
			aiBone*			 bone			 = mesh->mBones[boneIndex];
			std::string		 jointName		 = bone->mName.C_Str();
			JointWeightData& jointWeightData = modelData.skinClusterData[jointName];

			aiMatrix4x4	 bindPoseMatrixAssimp = bone->mOffsetMatrix.Inverse();
			aiVector3D	 scale, translate;
			aiQuaternion rotate;
			bindPoseMatrixAssimp.Decompose(scale, rotate, translate);

			CalyxEngine::Matrix4x4 bindPoseMatrix =
				CalyxEngine::MakeAffineMatrix({scale.x, scale.y, scale.z}, {rotate.x, -rotate.y, -rotate.z, rotate.w}, {-translate.x, translate.y, translate.z});
			jointWeightData.inverseBindPoseMatrix = CalyxEngine::Matrix4x4::Inverse(bindPoseMatrix);

			for(uint32_t weightIndex = 0; weightIndex < bone->mNumWeights; ++weightIndex) {
				jointWeightData.vertexWeights.push_back({bone->mWeights[weightIndex].mWeight, bone->mWeights[weightIndex].mVertexId});
			}
		}
	}

	// アニメーション(サンプル)
	if(scene->HasAnimations()) {
		aiAnimation* aiAnim = scene->mAnimations[0];
		Animation	 animation;
		float		 ticksPerSecond = (float)(aiAnim->mTicksPerSecond != 0 ? aiAnim->mTicksPerSecond : 25.0f);
		animation.duration			= (float)(aiAnim->mDuration / ticksPerSecond);

		for(unsigned int channelIdx = 0; channelIdx < aiAnim->mNumChannels; ++channelIdx) {
			aiNodeAnim*	  nodeAnim = aiAnim->mChannels[channelIdx];
			NodeAnimation nodeAnimation;

			// Translation Key
			for(uint32_t keyIndex = 0; keyIndex < nodeAnim->mNumPositionKeys; ++keyIndex) {
				aiVectorKey&	keyAssimp = nodeAnim->mPositionKeys[keyIndex];
				KeyframeVector3 keyframe;
				keyframe.time  = static_cast<float>(keyAssimp.mTime / aiAnim->mTicksPerSecond);
				keyframe.value = {-keyAssimp.mValue.x, keyAssimp.mValue.y, keyAssimp.mValue.z}; // 右手→左手
				nodeAnimation.translate.keyframes.push_back(keyframe);
			}

			// Rotation Key
			for(uint32_t keyIndex = 0; keyIndex < nodeAnim->mNumRotationKeys; ++keyIndex) {
				aiQuatKey&		   keyAssimp = nodeAnim->mRotationKeys[keyIndex];
				KeyframeQuaternion keyframe;
				keyframe.time  = static_cast<float>(keyAssimp.mTime / aiAnim->mTicksPerSecond);
				keyframe.value = {keyAssimp.mValue.x, -keyAssimp.mValue.y, -keyAssimp.mValue.z, keyAssimp.mValue.w}; // 右手→左手
				nodeAnimation.rotate.keyframes.push_back(keyframe);
			}

			// Scaling Key
			for(uint32_t keyIndex = 0; keyIndex < nodeAnim->mNumScalingKeys; ++keyIndex) {
				aiVectorKey&	keyAssimp = nodeAnim->mScalingKeys[keyIndex];
				KeyframeVector3 keyframe;
				keyframe.time  = static_cast<float>(keyAssimp.mTime / aiAnim->mTicksPerSecond);
				keyframe.value = {keyAssimp.mValue.x, keyAssimp.mValue.y, keyAssimp.mValue.z}; // スケールはそのまま
				nodeAnimation.scale.keyframes.push_back(keyframe);
			}
			std::string nodeName(nodeAnim->mNodeName.C_Str());
			animation.nodeAnimations[nodeName] = nodeAnimation;
		}
		modelData.animation = animation;
	}

	// スケルトン構築
	Node rootNode	   = ConvertAssimpNode(scene->mRootNode);
	modelData.skeleton = CreateSkeleton(rootNode);

	return modelData;
}

//----------------------------------------------------------------------------
// GPUリソース作成 (ProcessLoadingTasksから呼ばれる)
//----------------------------------------------------------------------------
void ModelManager::CreateGpuResources(const std::string&, ModelData& model) {
	auto device = GraphicsGroup::GetInstance()->GetDevice();

	const UINT vCount = (UINT)model.meshResource.Vertices().size();
	const UINT iCount = (UINT)model.meshResource.Indices().size();
	if(vCount == 0 || iCount == 0) return;

	auto& vb = model.meshResource.VertexBuffer();
	auto& ib = model.meshResource.IndexBuffer();

	vb.Initialize(device, vCount);
	vb.TransferVectorData(model.meshResource.Vertices());

	ib.Initialize(device, iCount);
	ib.TransferVectorData(model.meshResource.Indices());
}

//----------------------------------------------------------------------------
// メッシュ読み込み
//----------------------------------------------------------------------------
void ModelManager::LoadMesh(const aiMesh* mesh, ModelData& modelData) {
	uint32_t baseVertex = static_cast<uint32_t>(modelData.meshResource.Vertices().size());
	uint32_t indexStart = static_cast<uint32_t>(modelData.meshResource.Indices().size());

	// 初期AABBを極端な値に
	CalyxEngine::Vector3 minPos = {FLT_MAX, FLT_MAX, FLT_MAX};
	CalyxEngine::Vector3 maxPos = {-FLT_MAX, -FLT_MAX, -FLT_MAX};

	for(unsigned int i = 0; i < mesh->mNumVertices; ++i) {
		VertexPosUvN vertex{};
		vertex.position = {-mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z, 1.0f};
		if(mesh->HasNormals()) {
			vertex.normal = {-mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z};
		}
		if(mesh->HasTextureCoords(0)) {
			vertex.texcoord.x = mesh->mTextureCoords[0][i].x;
			vertex.texcoord.y = mesh->mTextureCoords[0][i].y;
		}
		modelData.meshResource.data.vertices.push_back(vertex);

		// AABB更新用の min/max 反映
		CalyxEngine::Vector3 pos = {vertex.position.x, vertex.position.y, vertex.position.z};
		minPos				   = CalyxEngine::Vector3::Min(minPos, pos);
		maxPos				   = CalyxEngine::Vector3::Max(maxPos, pos);
	}

	for(unsigned int i = 0; i < mesh->mNumFaces; ++i) {
		const aiFace& face = mesh->mFaces[i];
		modelData.meshResource.data.indices.push_back(baseVertex + face.mIndices[0]);
		modelData.meshResource.data.indices.push_back(baseVertex + face.mIndices[2]);
		modelData.meshResource.data.indices.push_back(baseVertex + face.mIndices[1]);
	}

	const uint32_t indexCount = static_cast<uint32_t>(modelData.meshResource.Indices().size()) - indexStart;
	if(indexCount > 0) {
		modelData.meshResource.SubMeshes().push_back({
			indexStart,
			indexCount,
			mesh->mMaterialIndex});
	}

	// ローカルAABBを格納
	if(modelData.localAABB.min_ == CalyxEngine::Vector3{} && modelData.localAABB.max_ == CalyxEngine::Vector3{}) {
		modelData.localAABB.Initialize(minPos, maxPos);
	} else {
		// モデル全体の AABB を統合（複数メッシュ時）
		CalyxEngine::Vector3 mergedMin = CalyxEngine::Vector3::Min(modelData.localAABB.min_, minPos);
		CalyxEngine::Vector3 mergedMax = CalyxEngine::Vector3::Max(modelData.localAABB.max_, maxPos);
		modelData.localAABB.Initialize(mergedMin, mergedMax);
	}
}

//----------------------------------------------------------------------------
// マテリアル読み込み
//----------------------------------------------------------------------------
void ModelManager::LoadMaterials(const aiScene* scene, const std::filesystem::path& modelDirectory, ModelData& modelData) {
	modelData.meshResource.Materials().clear();

	auto normalizeTexturePath = [](const aiString& texPath) -> std::string {
		std::istringstream stream(texPath.C_Str());
		std::string token;
		std::string texturePath;
		while(stream >> token) {
			if(!token.empty() && token[0] == '-') {
				continue;
			}
			texturePath = token;
		}
		return texturePath.empty() ? texPath.C_Str() : texturePath;
	};

	auto resolveTexturePath = [&](const aiString& texPath) -> std::string {
		std::error_code ec;
		std::filesystem::path assetRoot = std::filesystem::weakly_canonical("Resources/Assets", ec);
		if(ec) {
			assetRoot = std::filesystem::path("Resources/Assets");
			ec.clear();
		}
		std::filesystem::path path(normalizeTexturePath(texPath));
		if(path.is_absolute()) {
			auto rel = std::filesystem::relative(path, assetRoot, ec);
			if(!ec) return rel.generic_string();
			return path.generic_string();
		}

		std::filesystem::path full = std::filesystem::weakly_canonical(modelDirectory / path, ec);
		if(ec) {
			full = modelDirectory / path;
		}

		auto rel = std::filesystem::relative(full, assetRoot, ec);
		if(!ec) return rel.generic_string();

		return full.generic_string();
	};

	const unsigned int materialCount = scene && scene->HasMaterials() ? scene->mNumMaterials : 0;
	if(materialCount == 0) {
		MaterialData fallback{};
		fallback.textureFilePath = "textures/white1x1.dds";
		modelData.meshResource.Materials().push_back(fallback);
		modelData.meshResource.Material() = fallback;
		return;
	}

	for(unsigned int materialIndex = 0; materialIndex < materialCount; ++materialIndex) {
		MaterialData materialData{};
		materialData.textureFilePath = "textures/white1x1.dds";

		const aiMaterial* material = scene->mMaterials[materialIndex];
		if(material) {
			aiString texPath;
			if(material->GetTexture(aiTextureType_DIFFUSE, 0, &texPath) == AI_SUCCESS) {
				materialData.textureFilePath = resolveTexturePath(texPath);
			}
			LoadUVTransform(material, materialData);
		}

		modelData.meshResource.Materials().push_back(materialData);
	}

	modelData.meshResource.Material() = modelData.meshResource.Materials().front();
}

//----------------------------------------------------------------------------
// UV変換情報
//----------------------------------------------------------------------------
void ModelManager::LoadUVTransform(const aiMaterial* material, MaterialData& outMaterial) {
	aiUVTransform transformData;
	if(material->Get(AI_MATKEY_UVTRANSFORM(aiTextureType_DIFFUSE, 0), transformData) == AI_SUCCESS) {
		outMaterial.uv_offset = {transformData.mTranslation.x, transformData.mTranslation.y, 0.0f};
		outMaterial.uv_scale  = {transformData.mScaling.x, transformData.mScaling.y, 1.0f};
	} else {
		outMaterial.uv_offset = {0, 0, 0};
		outMaterial.uv_scale  = {1, 1, 1};
	}
}

void ModelManager::LoadSkinData([[maybe_unused]] const aiMesh* mesh, [[maybe_unused]] ModelData& modelData) {}

//----------------------------------------------------------------------------
// アニメーション評価サンプル
//----------------------------------------------------------------------------
CalyxEngine::Vector3 ModelManager::Evaluate(const AnimationCurve<CalyxEngine::Vector3>& curve, float time) {
	const auto& keyframes = curve.keyframes;
	if(keyframes.empty()) {
		return {0, 0, 0};
	}
	if(time <= keyframes.front().time) {
		return keyframes.front().value;
	}
	if(time >= keyframes.back().time) {
		return keyframes.back().value;
	}
	for(int i = 0; i < (int)keyframes.size() - 1; ++i) {
		float t0 = keyframes[i].time;
		float t1 = keyframes[i + 1].time;
		if(time >= t0 && time <= t1) {
			float localT = (time - t0) / (t1 - t0);
			return CalyxEngine::Vector3::Lerp(keyframes[i].value, keyframes[i + 1].value, localT);
		}
	}
	return keyframes.back().value;
}

CalyxEngine::Quaternion ModelManager::Evaluate(const AnimationCurve<CalyxEngine::Quaternion>& curve, float time) {
	const auto& keyframes = curve.keyframes;
	if(keyframes.empty()) {
		return {0, 0, 0, 1};
	}
	if(time <= keyframes.front().time) {
		return keyframes.front().value;
	}
	if(time >= keyframes.back().time) {
		return keyframes.back().value;
	}
	for(int i = 0; i < (int)keyframes.size() - 1; ++i) {
		float t0 = keyframes[i].time;
		float t1 = keyframes[i + 1].time;
		if(time >= t0 && time <= t1) {
			float localT = (time - t0) / (t1 - t0);
			return CalyxEngine::Quaternion::Slerp(keyframes[i].value, keyframes[i + 1].value, localT);
		}
	}
	return keyframes.back().value;
}
