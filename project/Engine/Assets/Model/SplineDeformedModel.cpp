#include "SplineDeformedModel.h"

#include <Engine/Assets/Manager/AssetManager.h>
#include <Engine/Assets/Model/ModelManager.h>
#include <Engine/Foundation/Math/Vector4.h>
#include <Engine/Graphics/Context/GraphicsGroup.h>

#include <algorithm>
#include <cfloat>
#include <cmath>

SplineDeformedModel::SplineDeformedModel() {
	modelData_ = nullptr;
	Initialize();
}

SplineDeformedModel::SplineDeformedModel(const std::string& sourceFile) {
	modelData_ = nullptr;
	Initialize();
	SetSourceModel(sourceFile);
}

void SplineDeformedModel::Initialize() {
	CreateMaterialBuffer();
	Map();
}

bool SplineDeformedModel::SetSourceModel(const std::string& sourceFile) {
	// 同じAssetが既に利用可能ならCPU/GPUデータの再構築を避ける。
	if(fileName_ == sourceFile && sourceReady_) {
		return true;
	}
	if(fileName_ == sourceFile) {
		return LoadSourceIfReady();
	}

	// Source変更時は旧Modelへの非所有参照を先に無効化し、非同期ロード完了まで描画させない。
	fileName_ = sourceFile;
	sourceReady_ = false;
	buffersReady_ = false;
	ownedModelData_.reset();
	modelData_ = nullptr;
	originalVertices_.clear();

	return LoadSourceIfReady();
}

bool SplineDeformedModel::LoadSourceIfReady() {
	if(sourceReady_) return true;
	if(fileName_.empty()) return false;

	auto* assetManager = CalyxEngine::AssetManager::GetInstance();
	if(!assetManager || !assetManager->GetModelManager()) return false;

	auto* modelManager = assetManager->GetModelManager();
	if(!modelManager->IsModelLoaded(fileName_)) {
		// AssetロードはManagerへ依頼し、このフレームでは未準備として呼び出し側へ通知する。
		modelManager->LoadModel(fileName_);
		return false;
	}

	ModelData& source = modelManager->GetModelData(fileName_);
	if(source.meshResource.Vertices().empty() || source.meshResource.Indices().empty()) {
		return false;
	}

	// 共有Assetを直接変形しないよう、変形対象のMesh情報を専用ModelDataへ複製する。
	ownedModelData_ = std::make_unique<ModelData>();
	ownedModelData_->meshResource.data.vertices = source.meshResource.Vertices();
	ownedModelData_->meshResource.data.indices = source.meshResource.Indices();
	ownedModelData_->meshResource.data.material = source.meshResource.Material();
	ownedModelData_->meshResource.data.materials = source.meshResource.Materials();
	ownedModelData_->meshResource.data.subMeshes = source.meshResource.SubMeshes();
	ownedModelData_->meshResource.topology = source.meshResource.topology;
	ownedModelData_->localAABB = source.localAABB;

	// Rebuildを繰り返しても変形誤差を累積させないため、常に元頂点から計算する。
	originalVertices_ = ownedModelData_->meshResource.Vertices();
	modelData_ = ownedModelData_.get();

	// 複製した頂点・Indexは共有ModelのBufferを参照できないため、専用GPU Bufferへ転送する。
	ID3D12Device* device = GraphicsGroup::GetInstance()->GetDevice().Get();
	modelData_->meshResource.VertexBuffer().Initialize(device, UINT(modelData_->meshResource.Vertices().size()));
	modelData_->meshResource.VertexBuffer().TransferVectorData(modelData_->meshResource.Vertices());
	modelData_->meshResource.IndexBuffer().Initialize(device, UINT(modelData_->meshResource.Indices().size()));
	modelData_->meshResource.IndexBuffer().TransferVectorData(modelData_->meshResource.Indices());
	buffersReady_ = true;

	OnModelLoaded();
	sourceReady_ = true;
	return true;
}

bool SplineDeformedModel::Rebuild(const SplineData& spline, Axis axis, float radiusScale, float distanceOffset) {
	if(!LoadSourceIfReady()) return false;
	if(!modelData_ || originalVertices_.empty() || spline.TotalLength() <= 0.0f || spline.SegmentCount() <= 0) {
		return false;
	}

	// 指定軸をSpline進行方向、残り2軸を断面方向として扱う。
	const int along = AxisIndex(axis);
	const int perpA = PerpIndexA(axis);
	const int perpB = PerpIndexB(axis);

	// Meshの軸方向範囲と断面中心を求め、頂点位置をSpline距離へ正規化できるようにする。
	float minAlong = FLT_MAX;
	float maxAlong = -FLT_MAX;
	float centerA = 0.0f;
	float centerB = 0.0f;
	for(const auto& vertex : originalVertices_) {
		const CalyxEngine::Vector3 local{vertex.position.x, vertex.position.y, vertex.position.z};
		minAlong = (std::min)(minAlong, local[along]);
		maxAlong = (std::max)(maxAlong, local[along]);
		centerA += local[perpA];
		centerB += local[perpB];
	}
	centerA /= static_cast<float>(originalVertices_.size());
	centerB /= static_cast<float>(originalVertices_.size());

	// ゼロ長Meshでも除算が破綻しない下限を設け、毎回未変形頂点へ戻してから再構築する。
	const float length = (std::max)(0.0001f, maxAlong - minAlong);
	auto& vertices = modelData_->meshResource.Vertices();
	vertices = originalVertices_;

	CalyxEngine::Vector3 prevRight = CalyxEngine::Vector3::Zero();
	for(size_t i = 0; i < vertices.size(); ++i) {
		const VertexPosUvN& src = originalVertices_[i];
		const CalyxEngine::Vector3 local{src.position.x, src.position.y, src.position.z};

		// 元Mesh上の軸位置をSpline上の弧長へ変換し、均一な距離基準で配置する。
		const float normalized = (local[along] - minAlong) / length;
		const float distance = normalized * spline.TotalLength() + distanceOffset;
		const float t = spline.DistanceToT(distance);

		CalyxEngine::Vector3 forward = spline.Tangent(t);
		if(forward.LengthSquared() <= 1e-8f) {
			forward = CalyxEngine::Vector3::Forward();
		}

		// 接線とUpが平行な場合は外積が退化するため、代替軸を選んで局所Basisを安定化する。
		CalyxEngine::Vector3 upSeed = CalyxEngine::Vector3::Up();
		if(std::abs(CalyxEngine::Vector3::Dot(forward, upSeed)) > 0.95f) {
			upSeed = {1.0f, 0.0f, 0.0f};
		}

		// 隣接頂点でRightの符号を揃え、断面が突然反転するTwistを抑制する。
		CalyxEngine::Vector3 right = CalyxEngine::Vector3::Cross(upSeed, forward).Normalize();
		if(prevRight.LengthSquared() > 1e-8f && CalyxEngine::Vector3::Dot(prevRight, right) < 0.0f) {
			right = -right;
		}
		prevRight = right;

		CalyxEngine::Vector3 up = CalyxEngine::Vector3::Cross(forward, right).Normalize();
		const CalyxEngine::Vector3 center = spline.Evaluate(t);

		// 元断面の中心からの距離だけを局所Basisへ再配置し、Mesh断面形状を維持する。
		const float offsetA = (local[perpA] - centerA) * radiusScale;
		const float offsetB = (local[perpB] - centerB) * radiusScale;
		const CalyxEngine::Vector3 deformed = center + right * offsetA + up * offsetB;
		vertices[i].position = {deformed.x, deformed.y, deformed.z, src.position.w};

		// 法線にも同じBasis変換を適用し、変形後のLighting方向を頂点位置と一致させる。
		const CalyxEngine::Vector3 nLocal = src.normal;
		const CalyxEngine::Vector3 n = (forward * nLocal[along] + right * nLocal[perpA] + up * nLocal[perpB]);
		vertices[i].normal = n.LengthSquared() > 1e-8f ? n.Normalize() : src.normal;
	}

	// CPU頂点変更後にCulling境界とGPU Bufferを同期し、旧形状のBLASは再構築対象にする。
	RecalculateLocalAABB();
	UploadDeformedVertices();
	blasBuilt_ = false;
	return true;
}

void SplineDeformedModel::UploadDeformedVertices() {
	if(!buffersReady_ || !modelData_ || modelData_->meshResource.Vertices().empty()) return;
	modelData_->meshResource.VertexBuffer().TransferVectorData(modelData_->meshResource.Vertices());
}

void SplineDeformedModel::RecalculateLocalAABB() {
	if(!modelData_ || modelData_->meshResource.Vertices().empty()) return;

	// 変形後の全頂点から境界を再計算し、旧Source ModelのBoundsによる誤Cullingを防ぐ。
	CalyxEngine::Vector3 minPos{FLT_MAX, FLT_MAX, FLT_MAX};
	CalyxEngine::Vector3 maxPos{-FLT_MAX, -FLT_MAX, -FLT_MAX};
	for(const auto& vertex : modelData_->meshResource.Vertices()) {
		const CalyxEngine::Vector3 p{vertex.position.x, vertex.position.y, vertex.position.z};
		minPos = CalyxEngine::Vector3::Min(minPos, p);
		maxPos = CalyxEngine::Vector3::Max(maxPos, p);
	}
	modelData_->localAABB.Initialize(minPos, maxPos);
}

void SplineDeformedModel::Draw(const WorldTransform& transform) {
	if(!modelData_) return;
	Model::Draw(transform);
}

void SplineDeformedModel::ShowImGuiInterface() {
	BaseModel::ShowImGuiInterface();
}

void SplineDeformedModel::CreateMaterialBuffer() {
	ID3D12Device* device = GraphicsGroup::GetInstance()->GetDevice().Get();
	materialBuffer_.Initialize(device);
}

void SplineDeformedModel::MaterialBufferMap() {
	TransferMaterial();
}

void SplineDeformedModel::Map() {
	MaterialBufferMap();
}

int SplineDeformedModel::AxisIndex(Axis axis) {
	return static_cast<int>(axis);
}

int SplineDeformedModel::PerpIndexA(Axis axis) {
	switch(axis) {
	case Axis::X:
		return 2;
	case Axis::Y:
		return 0;
	case Axis::Z:
	default:
		return 0;
	}
}

int SplineDeformedModel::PerpIndexB(Axis axis) {
	switch(axis) {
	case Axis::X:
		return 1;
	case Axis::Y:
		return 2;
	case Axis::Z:
	default:
		return 1;
	}
}
