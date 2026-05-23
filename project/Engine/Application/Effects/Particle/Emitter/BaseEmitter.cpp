#include "BaseEmitter.h"

#include "Engine/Assets/Database/AssetDatabase.h"
#include "Engine/Assets/Manager/AssetManager.h"
#include "Engine/Foundation/Math/MathUtil.h"
#include "Engine/Foundation/Utility/Converter/EnumConverter.h"

#include <algorithm>
#include <cmath>
#include <iostream>

namespace CalyxEngine {
	BaseEmitter::BaseEmitter() = default;

	void BaseEmitter::TransferParticleDataToGPU() {
		if(units_.empty()) return;
		std::vector<ParticleConstantData> gpuUnits;
		for(const auto& fx : units_) {
			if(fx.alive) {
				gpuUnits.push_back({fx.position, fx.scale, fx.color});
			}
		}
		if(!gpuUnits.empty()) {
			instanceBuffer_.TransferVectorData(gpuUnits);
		}
	}

	MeshResource& BaseEmitter::GetMeshResource() {
		// メッシュを返す
		// プリミティブ形状じゃない場合モデルからメッシュを取得
		if(!primitive_.has_value()) {
			return AssetManager::GetInstance()->GetModelManager()->GetMeshResource(modelPath);
		}

		// プリミティブ形状の場合は内部メッシュを返す
		// TODO: プリミティブ形状のメッシュ生成を実装する
		// TODO: 形状ごとにメッシュを返すファクトリを実装する
		// NOTE: 仮に plane メッシュを返すようにしておく
		return AssetManager::GetInstance()->GetModelManager()->GetMeshResource(modelPath);
	}

	CalyxEngine::Vector3 BaseEmitter::GenerateSpawnPosition() {
		using namespace CalyxEngine;

		const Vector3 absScale{
			(std::max)(std::abs(worldScale_.x), 0.0001f),
			(std::max)(std::abs(worldScale_.y), 0.0001f),
			(std::max)(std::abs(worldScale_.z), 0.0001f)};

		auto rotateLocal = [&](const Vector3& localOffset) {
			return position_ + Quaternion::RotateVector(localOffset, worldRotation_);
		};

		switch(shape_) {
		case EmitterShape::Point:
			return position_;

		case EmitterShape::Sphere: {
			const float radius = (std::max)(shapeRadius_, 0.0f);
			Vector3 dir = Random::GenerateUnitVector3();
			const float t = Random::Generate<float>(0.0f, 1.0f);
			const float r = radius * std::cbrt(t);
			Vector3 localOffset{dir.x * r * absScale.x, dir.y * r * absScale.y, dir.z * r * absScale.z};
			return rotateLocal(localOffset);
		}

		case EmitterShape::Box: {
			const Vector3 halfSize{
				(std::max)(std::abs(shapeSize_.x), 0.0f) * absScale.x,
				(std::max)(std::abs(shapeSize_.y), 0.0f) * absScale.y,
				(std::max)(std::abs(shapeSize_.z), 0.0f) * absScale.z};
			Vector3 localOffset(
				Random::Generate<float>(-halfSize.x, halfSize.x),
				Random::Generate<float>(-halfSize.y, halfSize.y),
				Random::Generate<float>(-halfSize.z, halfSize.z));
			return rotateLocal(localOffset);
		}

		case EmitterShape::Circle: {
			const float radius = (std::max)(shapeRadius_, 0.0f);
			const float theta = Random::Generate<float>(0.0f, kTwoPi);
			const float r = radius * std::sqrt(Random::Generate<float>(0.0f, 1.0f));
			Vector3 localOffset{std::cos(theta) * r * absScale.x, 0.0f, std::sin(theta) * r * absScale.z};
			return rotateLocal(localOffset);
		}

		case EmitterShape::Cone: {
			const float height = (std::max)(shapeRadius_, 0.0f) * absScale.y;
			const float angleRad = std::clamp(ToRadians(shapeAngle_), 0.0f, kPi * 0.5f);
			const float h = Random::Generate<float>(0.0f, height);
			const float maxR = h * std::tan(angleRad);
			const float theta = Random::Generate<float>(0.0f, kTwoPi);
			const float radial = maxR * std::sqrt(Random::Generate<float>(0.0f, 1.0f));
			Vector3 localOffset{
				std::cos(theta) * radial * absScale.x,
				h,
				std::sin(theta) * radial * absScale.z};
			return rotateLocal(localOffset);
		}
		}

		return position_;
	}

	bool BaseEmitter::LoadModelByGuid(const Guid& g) {
		if(!g.isValid()) return false;

		auto* db  = AssetDatabase::GetInstance();
		auto* rec = db->Get(g);
		if(!rec || rec->type != AssetType::Model) return false;

		// モデルマネージャーでロード
		modelPath  = rec->sourcePath.filename().string();
		modelGuid_ = g;

		// ModelManagerにロードを要求（未ロードの場合にのみ実際にロードされる）
		AssetManager::GetInstance()->GetModelManager()->LoadModel(modelPath);

		return true;
	}

} // namespace CalyxEngine