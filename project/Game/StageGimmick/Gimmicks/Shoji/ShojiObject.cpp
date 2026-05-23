#include "ShojiObject.h"

#include <Engine/Scene/Utility/SceneUtility.h>
#include <Engine/Objects/3D/Actor/Registry/SceneObjectRegistry.h>

REGISTER_SCENE_OBJECT(ShojiObject);

ShojiObject::ShojiObject(
	const std::string&		   modelName,
	std::optional<std::string> objectName)
	: StageGimmickObjectBase(modelName, objectName) {

	worldTransform_.translation.y += 1.5f;
	worldTransform_.translation.z += 5.0f;
}

void ShojiObject::ApplyDerivedConfigFromJson(const nlohmann::json&, const nlohmann::json* derived) {
	if(!derived) return;
	paperGuids_ = derived->value("paperGuids", std::array<Guid, 12>{});
}

void ShojiObject::ExtractDerivedConfigToJson(nlohmann::json&, nlohmann::json& derived) const {
	std::array<Guid, 12> guids = paperGuids_;
	for(size_t i = 0; i < paperObjs_.size(); ++i) {
		if(auto paper = paperObjs_[i]) {
			guids[i] = paper->GetGuid();
		}
	}
	derived["paperGuids"] = guids;
}

void ShojiObject::RemapSceneObjectReferences(const std::unordered_map<Guid, Guid>& guidMap) {
	for(auto& guid : paperGuids_) {
		auto it = guidMap.find(guid);
		if(it != guidMap.end()) {
			guid = it->second;
		}
	}
}

void ShojiObject::ObjectInitialize() {

	// コライダーの初期化
	BaseGameObject::InitializeCollider(ColliderKind::Box);
	if(collider_) {
		collider_->SetType(ColliderType::Type_EnemyAttack);
		collider_->SetTargetType(ColliderType::Type_Player);
		collider_->SetOwner(this);
		collider_->SetCollisionEnabled(true);
	}
	worldTransform_.scale = CalyxEngine::Vector3::One() * param_.shojiScale;
	worldTransform_.inheritScale = false;

	offsetX_   = worldTransform_.translation.x;
	velocityX_ = param_.openVelocityX;
}

void ShojiObject::ObjectUpdate(float dt) {

	if(isOpen_ && !isStop_) {
		float w = param_.wOpen;
		if(worldTransform_.translation.x > 0.0f) {
			velocityX_ += param_.openAccelerationX;
		} else {
			velocityX_ -= param_.openAccelerationX;
			w *= -1.0f;
		}
		worldTransform_.translation.x += velocityX_ * dt;

		if(worldTransform_.translation.x > 0.0f) {
			if(worldTransform_.translation.x > w) {
				worldTransform_.translation.x = w;
				isStop_ = true;
			}
		} else {
			if(worldTransform_.translation.x < w) {
				worldTransform_.translation.x = w;
				isStop_ = true;
			}
		}
	}
}

void ShojiObject::CreatePaperObjects() {

	const std::string objectName = "ShojiPaperObject";

	// GUIDで復元可能なら復元して再利用
	if(auto* ctx = SceneContext::Current()) {
		if(auto* library = ctx->GetObjectLibrary()) {
			for(size_t i = 0; i < paperGuids_.size(); ++i) {
				if(!paperGuids_[i].isValid()) {
					continue;
				}
				auto object = library->Find(paperGuids_[i]);
				if(!object || object->GetObjectClassName() != objectName) {
					continue;
				}
				auto paper = std::static_pointer_cast<ShojiPaperObject>(object);
				paper->SetParent(shared_from_this());
				paperObjs_[i] = paper;
			}
		}
	}

	// GUID未設定のスロットは子オブジェクトから補完
	for(const auto& child : GetChildren()) {
		auto paper = std::dynamic_pointer_cast<ShojiPaperObject>(child);
		if(!paper) {
			continue;
		}
		for(size_t i = 0; i < paperObjs_.size(); ++i) {
			if(paperObjs_[i]) {
				continue;
			}
			paperObjs_[i]  = paper;
			paperGuids_[i] = paper->GetGuid();
			break;
		}
	}

	for(size_t i = 0; i < paperObjs_.size(); ++i) {

		// 既に存在している場合は生成しない
		if(paperObjs_[i]) {
			continue;
		}
		auto paper = SceneAPI::Instantiate<ShojiPaperObject>("shojiPaper.obj", objectName);
		if(!paper) {
			continue;
		}
		float				 width	= static_cast<float>(i % 3) * param_.wInterval;
		float				 height = static_cast<float>(i / 3) * param_.hInterval;
		CalyxEngine::Vector3 center = param_.centerPos + CalyxEngine::Vector3{width, -height, 0.0f};
		paper->SetTranslate(center);
		paper->SetParent(shared_from_this());
		paper->Initialize();
		paperObjs_[i]  = paper;
		paperGuids_[i] = paper->GetGuid();
	}
}
