#include"MyFunc.h"

//engine
#include <Engine/Application/System/Environment.h>
#include <Engine/Assets/Model/Model.h>
#include <Engine/Foundation/Utility/Converter/ConvertString.h>
#include <Engine/Foundation/Utility/Func/CxUtils.h>
#include <Engine/Graphics/Camera/Manager/CameraManager.h>
#include <Engine/Graphics/Descriptor/DescriptorAllocator.h>
// c++
#include<cassert>
#include<cmath>
#include<fstream>
#include <numbers>
#include<sstream>
#include <filesystem>

// externals
#include<assimp/Importer.hpp>
#include<assimp/postprocess.h>


CalyxMath::Matrix4x4 MakeOrthographicMatrix(float l, float t, float r, float b, float nearClip, float farClip) {
	CalyxMath::Matrix4x4 result;
	result = {
		2 / (r - l), 0, 0, 0,
		0, 2 / (t - b), 0, 0,
		0, 0, 1 / (farClip - nearClip), 0,
		(l + r) / (l - r), (t + b) / (b - t), nearClip / (nearClip - farClip), 1
	};
	return result;
}

Microsoft::WRL::ComPtr<ID3D12Resource>CreateBufferResource(Microsoft::WRL::ComPtr<ID3D12Device> device, size_t sizeInBytes) {
	// 頂点リソース用のヒープの設定
	D3D12_HEAP_PROPERTIES uploadHeapProperties{};
	uploadHeapProperties.Type = D3D12_HEAP_TYPE_UPLOAD;

	// 頂点リソースの設定
	D3D12_RESOURCE_DESC bufferResourceDesc{};
	// バッファリソース。テクスチャの場合はまた別の設定をする
	bufferResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	bufferResourceDesc.Width = sizeInBytes; // 指定されたサイズに設定
	// バッファの場合はこれらは1にする決まり
	bufferResourceDesc.Height = 1;
	bufferResourceDesc.DepthOrArraySize = 1;
	bufferResourceDesc.MipLevels = 1;
	bufferResourceDesc.SampleDesc.Count = 1;
	// バッファの場合はこれにする決まり
	bufferResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

	// 実際にリソースを作る
	Microsoft::WRL::ComPtr<ID3D12Resource> bufferResource = nullptr;
	device->CreateCommittedResource(&uploadHeapProperties, D3D12_HEAP_FLAG_NONE,
									&bufferResourceDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&bufferResource));

	return bufferResource;
}

D3D12_CPU_DESCRIPTOR_HANDLE GetCPUDescriptorHandle(ID3D12DescriptorHeap* descriptorHeap, uint32_t descriptorSize, uint32_t index) {
	D3D12_CPU_DESCRIPTOR_HANDLE handleCPU = descriptorHeap->GetCPUDescriptorHandleForHeapStart();
	handleCPU.ptr += (descriptorSize * index);
	return handleCPU;
}

D3D12_GPU_DESCRIPTOR_HANDLE GetGPUDescriptorHandle(ID3D12DescriptorHeap* descriptorHeap, uint32_t descriptorSize, uint32_t index) {
	D3D12_GPU_DESCRIPTOR_HANDLE handleGPU = descriptorHeap->GetGPUDescriptorHandleForHeapStart();
	handleGPU.ptr += (descriptorSize * index);
	return handleGPU;
}

ModelData LoadObjFile(const std::string& directoryPath, const std::string& filename) {
	Assimp::Importer importer;

	const std::vector<std::string> extensions = { ".gltf", ".obj" };
	std::string filePath;
	for (const auto& ext : extensions) {
		std::string tryPath = directoryPath + "/" + filename + "/" + filename + ext;
		std::ifstream file(tryPath);
		if (file.good()) {
			filePath = tryPath;
			break;
		}
	}

	assert(!filePath.empty() && "モデルファイル（.obj/.gltf）が見つかりません");
	// Assimpによるシーンの読み込み
	const aiScene* scene = importer.ReadFile(filePath, aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_CalcTangentSpace);
	assert(scene && scene->HasMeshes()); // 読み込みエラーやメッシュの有無を確認

	ModelData modelData;
	const aiMesh* mesh = scene->mMeshes[0]; // 最初のメッシュを取得

	// 頂点データの読み込み
	for (unsigned int i = 0; i < mesh->mNumVertices; i++) {
		VertexPosUvN vertex;

		// 位置データの取得
		vertex.position.x = mesh->mVertices[i].x;
		vertex.position.y = mesh->mVertices[i].y;
		vertex.position.z = mesh->mVertices[i].z;
		vertex.position.w = 1.0f;

		// 法線データの取得
		if (mesh->HasNormals()) {
			vertex.normal.x = mesh->mNormals[i].x;
			vertex.normal.y = mesh->mNormals[i].y;
			vertex.normal.z = mesh->mNormals[i].z;
		}

		// テクスチャ座標の取得
		if (mesh->HasTextureCoords(0)) {
			vertex.texcoord.x = mesh->mTextureCoords[0][i].x;
			vertex.texcoord.y = mesh->mTextureCoords[0][i].y;
		} else {
			vertex.texcoord = { 0.0f, 0.0f };
		}

		modelData.meshResource.Vertices().push_back(vertex);
	}

	// インデックスデータの読み込み
	for (unsigned int i = 0; i < mesh->mNumFaces; i++) {
		const aiFace& face = mesh->mFaces[i];
		assert(face.mNumIndices == 3); // 三角形のみを想定

		modelData.meshResource.Indices().push_back(face.mIndices[0]);
		modelData.meshResource.Indices().push_back(face.mIndices[1]);
		modelData.meshResource.Indices().push_back(face.mIndices[2]);
	}

	// skinCluster構築用のデータ取得
	for (uint32_t boneIndex = 0; boneIndex < mesh->mNumBones; ++boneIndex) {
		aiBone* bone = mesh->mBones[boneIndex];
		std::string jointName = bone->mName.C_Str();
		JointWeightData& jointWeightData = modelData.skinClusterData[jointName];

		// AssimpのOffsetMatrixは逆バインドポーズ行列として正しい（逆にしない！）
		aiMatrix4x4 offsetMatrixAssimp = bone->mOffsetMatrix;
		aiVector3D scale, translate;
		aiQuaternion rotate;
		offsetMatrixAssimp.Decompose(scale, rotate, translate);

		CalyxMath::Matrix4x4 inverseBindPoseMatrix = CalyxMath::MakeAffineMatrix(
			{ scale.x, scale.y, scale.z },
			{ rotate.x, -rotate.y, -rotate.z, rotate.w }, // 左手変換
			{ -translate.x, translate.y, translate.z }     // 左手変換
		);
		jointWeightData.inverseBindPoseMatrix = inverseBindPoseMatrix;

		for (uint32_t weightIndex = 0; weightIndex < bone->mNumWeights; ++weightIndex) {
			jointWeightData.vertexWeights.push_back({
				bone->mWeights[weightIndex].mWeight,
				bone->mWeights[weightIndex].mVertexId
													});
		}
	}

	// マテリアルの読み込み
	if (scene->HasMaterials()) {
		const aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];
		aiString texturePath;
		if (material->GetTexture(aiTextureType_DIFFUSE, 0, &texturePath) == AI_SUCCESS) {
			modelData.meshResource.Material().textureFilePath = texturePath.C_Str();
		} else {
			modelData.meshResource.Material().textureFilePath = "white1x1.dds";
		}
	}

	// スケルトン構築
	Node rootNode = ConvertAssimpNode(scene->mRootNode);
	modelData.skeleton = CreateSkeleton(rootNode);

	return modelData;
}

MaterialData LoadMaterialTemplateFile(const std::string& directoryPath, const std::string& filename) {

	// ファイルを開く
	std::ifstream file(directoryPath + "/" + filename);
	assert(file.is_open());// 失敗したらアサート

	MaterialData materialData;
	std::string line;

	while (std::getline(file, line)) {

		// まずobjファイルの行の先頭の識別子を読む
		std::string identifer;
		std::istringstream s(line);
		s >> identifer;

		if (identifer == "map_Kd") {// ファイル名

			std::string textureFilename;
			CalyxMath::Vector3 scale = { 1.0f,1.0f,1.0f };
			CalyxMath::Vector3 offset = { 0.0f,0.0f,0.0f };
			CalyxMath::Vector3 translate = { 0.0f,0.0f,0.0f };

			// ファイル名を格納
			while (s >> textureFilename) {
				if (textureFilename[0] == '-') {
					std::string option = textureFilename.substr(1);
					if (option == "s") {
						s >> scale.x >> scale.y >> scale.z;
					} else if (option == "o") {
						s >> offset.x >> offset.y >> offset.z;
					} else if (option == "t") {
						s >> translate.x >> translate.y >> translate.z;
					}
				} else {
					materialData.textureFilePath = textureFilename;
				}
			}

			materialData.uv_scale = scale;
			materialData.uv_offset = offset;
			materialData.uv_translate = translate;
		}
	}

	// テクスチャなしのモデルの場合
	if (materialData.textureFilePath == "") {
		materialData.textureFilePath = "white1x1.dds";
	}

	return materialData;
}

DirectX::ScratchImage LoadTextureImage(const std::string& filePath) {
	using namespace DirectX;

	ScratchImage image{};
	ScratchImage mipImages{};

	std::filesystem::path path(filePath);
	std::filesystem::path ddsPath = path;
	ddsPath.replace_extension(".dds");

	std::wstring filePathW;

	HRESULT hr = E_FAIL;
	bool useDDS = false;

	// もともとDDSを指定している
	// もしくは DDSファイルが存在する かつ 元のファイルが存在しない
	// ならDDSを使う
	bool originExists = std::filesystem::exists(path);
	if (path.extension() == ".dds" || (!originExists && std::filesystem::exists(ddsPath))) {
		useDDS = true;
		filePathW = ConvertString(ddsPath.string());
	} else {
		filePathW = ConvertString(filePath);
	}

	// ファイル形式に応じて読み込み
	if (useDDS) {
		hr = LoadFromDDSFile(filePathW.c_str(), DDS_FLAGS_NONE, nullptr, image);
	} else {
		hr = LoadFromWICFile(filePathW.c_str(), WIC_FLAGS_FORCE_SRGB, nullptr, image);
	}
	assert(SUCCEEDED(hr));

	// ミップマップ生成
	const TexMetadata& meta = image.GetMetadata();
	if (meta.width > 1 && meta.height > 1) {
		if (IsCompressed(meta.format)) {
			return image;
		} else {
			hr = GenerateMipMaps(
				image.GetImages(),
				image.GetImageCount(),
				meta,
				TEX_FILTER_SRGB,
				0,
				mipImages
			);
			assert(SUCCEEDED(hr));
			return mipImages;
		}
	}

	return image;
}


bool IsCollision(const AABB& aabb, const CalyxMath::Vector3& point) {
	// pointがaabbのminとmaxの範囲内にあるかチェック
	return (point.x >= aabb.min_.x && point.x <= aabb.max_.x) &&
		(point.y >= aabb.min_.y && point.y <= aabb.max_.y) &&
		(point.z >= aabb.min_.z && point.z <= aabb.max_.z);
}

/////////////////////////////////////////////////////////////////////////////////////////////
//							Animation
/////////////////////////////////////////////////////////////////////////////////////////////
//アニメーションデータを読み込む関数
Animation LoadAnimationFile(const std::string& directoryPath, const std::string& filename) {
	Animation animation;// アニメーションデータ
	Assimp::Importer importer;
	std::string filePath = directoryPath + "/"
		+ filename.substr(0, filename.find_last_of('.')) + "/"
		+ filename;

	const aiScene* scene = importer.ReadFile(filePath.c_str(), 0);
	assert(scene->mNumAnimations);// アニメーションがない場合はアサート
	aiAnimation* animationAssimp = scene->mAnimations[0];// 最初のアニメーションを取得
	animation.duration = float(animationAssimp->mDuration / animationAssimp->mTicksPerSecond);// アニメーションの長さを取得

	// assimpでは個々のanimationをchannelとして読んでいるからchannelの数だけループ
	for (uint32_t channelIndex = 0; channelIndex < animationAssimp->mNumChannels; ++channelIndex) {
		aiNodeAnim* nodeAnimationAssimp = animationAssimp->mChannels[channelIndex];// channelを取得
		NodeAnimation& nodeAnimation = animation.nodeAnimations[nodeAnimationAssimp->mNodeName.C_Str()];// ノードアニメーションを取得

		//translate
		for (uint32_t keyIndex = 0; keyIndex < nodeAnimationAssimp->mNumPositionKeys; ++keyIndex) {
			aiVectorKey& keyAssimp = nodeAnimationAssimp->mPositionKeys[keyIndex];// キーフレームを取得
			KeyframeVector3 keyframe;

			keyframe.time = float(keyAssimp.mTime / animationAssimp->mTicksPerSecond);// キーフレームの時間を取得
			keyframe.value = { -keyAssimp.mValue.x, keyAssimp.mValue.y, keyAssimp.mValue.z };// キーフレームの値を取得 //<右手->左手座標系に変換>
			nodeAnimation.translate.keyframes.push_back(keyframe);// ノードアニメーションに追加

		}

		//rotate
		for (uint32_t keyIndex = 0; keyIndex < nodeAnimationAssimp->mNumRotationKeys; ++keyIndex) {
			aiQuatKey& keyAssimp = nodeAnimationAssimp->mRotationKeys[keyIndex];
			KeyframeQuaternion keyframe;

			keyframe.time = float(keyAssimp.mTime / animationAssimp->mTicksPerSecond);
			// 右手->左手 (yとzの符号を反転)
			keyframe.value = { keyAssimp.mValue.x, -keyAssimp.mValue.y, -keyAssimp.mValue.z, keyAssimp.mValue.w };
			nodeAnimation.rotate.keyframes.push_back(keyframe);
		}

		//scale
		for (uint32_t keyIndex = 0; keyIndex < nodeAnimationAssimp->mNumScalingKeys; ++keyIndex) {
			aiVectorKey& keyAssimp = nodeAnimationAssimp->mScalingKeys[keyIndex];
			KeyframeVector3 keyframe;

			keyframe.time = float(keyAssimp.mTime / animationAssimp->mTicksPerSecond);
			keyframe.value = { keyAssimp.mValue.x, keyAssimp.mValue.y, keyAssimp.mValue.z };
			nodeAnimation.scale.keyframes.push_back(keyframe);
		}

	}
	return animation;
}

//ノードの情報を取得する関数
Skeleton CreateSkeleton(const Node& rootNode) {
	Skeleton skeleton;
	skeleton.root = CreateJoint(rootNode, {}, skeleton.joints);

	//名前とindexのマッピングを行いアクセスしやすくする
	for (const Joint& joint : skeleton.joints) {
		skeleton.jointMap.emplace(joint.name, joint.index);
	}
	return skeleton;
}

//ノードの情報を取得する関数
int32_t CreateJoint(const Node& node, const std::optional<int32_t>& parent, std::vector<Joint>& joints) {

	Joint joint;
	joint.name = node.name;
	joint.localMatrix = node.localMatrix;
	joint.skeletonSpaceMatrix = CalyxMath::Matrix4x4::MakeIdentity();
	joint.transform = node.transform;
	joint.index = static_cast<int32_t>(joints.size());	//現在登録されているjointの数をindexにする
	joint.parent = parent;

	//skeletonのjoint列に追加
	joints.push_back(joint);

	for (const Node& child : node.children) {
		//子jointを作成し、そのindexを登録
		int32_t childIndex = CreateJoint(child, joint.index, joints);
		joints[joint.index].children.push_back(childIndex);
	}
	//自身のindexを返す
	return joint.index;
}

Node ConvertAssimpNode(const aiNode* node) {
	Node result;

	aiVector3D scale, translate;
	aiQuaternion rotate;
	node->mTransformation.Decompose(scale, rotate, translate);

	result.transform.scale = { scale.x, scale.y, scale.z };
	result.transform.rotate = { rotate.x, -rotate.y, -rotate.z, rotate.w }; // 左手系
	result.transform.translate = { -translate.x, translate.y, translate.z }; // 左手系

	result.localMatrix =
		CalyxMath::MakeAffineMatrix(result.transform.scale, result.transform.rotate, result.transform.translate);

	result.name = node->mName.C_Str();
	result.children.resize(node->mNumChildren);

	for (uint32_t childIndex = 0; childIndex < node->mNumChildren; ++childIndex) {
		result.children[childIndex] = ConvertAssimpNode(node->mChildren[childIndex]);
	}

	return result;
}


SkinCluster CreateSkinCluster(const Microsoft::WRL::ComPtr<ID3D12Device>& device,
							  const Skeleton& skeleton, const ModelData& modelData){
	SkinCluster skinCluster;

	//===================================================================*/
	//	palette用のリソースの確保
	//===================================================================*/
	skinCluster.paletteResource = CreateBufferResource(device, sizeof(WellForGPU) * skeleton.joints.size());

	WellForGPU* mappedPalette = nullptr;
	skinCluster.paletteResource->Map(0, nullptr, reinterpret_cast< void** >(&mappedPalette));
	skinCluster.mappedPalette = {mappedPalette, skeleton.joints.size()};

	DescriptorHandle handle = DescriptorAllocator::Allocate(DescriptorUsage::CbvSrvUav);
	skinCluster.paletteSrvHandle.first = handle.cpu;
	skinCluster.paletteSrvHandle.second = handle.gpu;

	//===================================================================*/
	//	palette用のSRV作成
	//===================================================================*/
	D3D12_SHADER_RESOURCE_VIEW_DESC paletteSrvDesc {};
	paletteSrvDesc.Format = DXGI_FORMAT_UNKNOWN;
	paletteSrvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	paletteSrvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
	paletteSrvDesc.Buffer.FirstElement = 0;
	paletteSrvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;
	paletteSrvDesc.Buffer.NumElements = static_cast< UINT >(skeleton.joints.size());
	paletteSrvDesc.Buffer.StructureByteStride = sizeof(WellForGPU);

	device->CreateShaderResourceView(
		skinCluster.paletteResource.Get(),
		&paletteSrvDesc,
		skinCluster.paletteSrvHandle.first // ← CPUハンドル
	);

	//===================================================================*/
	//	influence用リソース確保 + 初期化
	//===================================================================*/
	skinCluster.influenceResource = CreateBufferResource(device, sizeof(VertexInfluence) * modelData.meshResource.Vertices().size());
	VertexInfluence* mappedInfluence = nullptr;
	skinCluster.influenceResource->Map(0, nullptr, reinterpret_cast< void** >(&mappedInfluence));
	std::memset(mappedInfluence, 0, sizeof(VertexInfluence) * modelData.meshResource.Vertices().size());
	skinCluster.mappedInfluence = {mappedInfluence, modelData.meshResource.Vertices().size()};

	//===================================================================*/
	//	influence用VBVの構築
	//===================================================================*/
	skinCluster.influenceBufferView.BufferLocation = skinCluster.influenceResource->GetGPUVirtualAddress();
	skinCluster.influenceBufferView.SizeInBytes = UINT(sizeof(VertexInfluence) * modelData.meshResource.Vertices().size());
	skinCluster.influenceBufferView.StrideInBytes = sizeof(VertexInfluence);

	//===================================================================*/
	//	inverseBindPoseMatrix（単位行列で初期化）
	//===================================================================*/
	skinCluster.inverseBindPoseMatrices.resize(skeleton.joints.size());
	std::generate(
		skinCluster.inverseBindPoseMatrices.begin(),
		skinCluster.inverseBindPoseMatrices.end(),
		[] (){ return CalyxMath::Matrix4x4::MakeIdentity(); }
	);

	//===================================================================*/
	//	influenceの割り当て
	//===================================================================*/
	for (const auto& jointWeight : modelData.skinClusterData){
		auto it = skeleton.jointMap.find(jointWeight.first);
		if (it == skeleton.jointMap.end()) continue;

		skinCluster.inverseBindPoseMatrices[it->second] = jointWeight.second.inverseBindPoseMatrix;

		for (const auto& vertexWeight : jointWeight.second.vertexWeights){
			auto& currentInfluence = skinCluster.mappedInfluence[vertexWeight.vertexIndex];
			for (uint32_t index = 0; index < kNumMaxInfluence; ++index){
				if (currentInfluence.weights[index] == 0.0f){
					currentInfluence.weights[index] = vertexWeight.weight;
					currentInfluence.jointIndices[index] = it->second;
					break;
				}
			}
		}
	}

	return skinCluster;
}


CalyxMath::Matrix4x4 MakeYAxisBillboard(const CalyxMath::Matrix4x4& cameraMatrix) {
	CalyxMath::Vector3 camZ = { cameraMatrix.m[0][2], 0.0f, cameraMatrix.m[2][2] };
	camZ = camZ.Normalize();
	CalyxMath::Vector3 camX = CalyxMath::Vector3::Cross({ 0, 1, 0 }, camZ).Normalize();
	CalyxMath::Vector3 camY = CalyxMath::Vector3::Cross(camZ, camX);
	CalyxMath::Vector3 cam = (camX, camY, camZ);
	return CalyxMath::MakeAffineMatrix(CalyxMath::Vector3::One(), cam, {});
}

CalyxMath::Matrix4x4 MakeXAxisBillboard(const CalyxMath::Matrix4x4& cameraMatrix) {
	CalyxMath::Vector3 camZ = { 0.0f, cameraMatrix.m[1][2], cameraMatrix.m[2][2] };
	camZ = camZ.Normalize();
	CalyxMath::Vector3 camY = CalyxMath::Vector3::Cross(camZ, { 1, 0, 0 }).Normalize();
	CalyxMath::Vector3 camX = CalyxMath::Vector3::Cross(camY, camZ);
	CalyxMath::Vector3 cam = (camX, camY, camZ);
	return CalyxMath::MakeAffineMatrix(CalyxMath::Vector3::One(), cam, {});
}

CalyxMath::Matrix4x4 MakeZAxisBillboard(const CalyxMath::Matrix4x4& cameraMatrix) {
	CalyxMath::Vector3 camY = { cameraMatrix.m[0][1], cameraMatrix.m[1][1], 0.0f };
	camY = camY.Normalize();
	CalyxMath::Vector3 camX = CalyxMath::Vector3::Cross(camY, { 0, 0, 1 }).Normalize();
	CalyxMath::Vector3 camZ = CalyxMath::Vector3::Cross(camX, camY);
	CalyxMath::Vector3 cam = (camX, camY, camZ);
	return CalyxMath::MakeAffineMatrix(CalyxMath::Vector3::One(), cam, {});
}