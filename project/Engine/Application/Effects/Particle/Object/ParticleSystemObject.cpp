#include "ParticleSystemObject.h"
#include <Engine/Application/Effects/Particle/Emitter/FxEmitter.h>
#include <Engine/Assets/Texture/TextureManager.h>
#include <Engine/Objects/3D/Actor/Registry/SceneObjectRegistry.h>
#include <Engine/Scene/Context/SceneContext.h>
#include <Engine/System/Event/EventBus.h>
#include <iostream>

namespace {
	CalyxMath::Vector3 ExtractWorldScale(const CalyxMath::Matrix4x4& m) {
		return {
			CalyxMath::Vector3(m.m[0][0], m.m[0][1], m.m[0][2]).Length(),
			CalyxMath::Vector3(m.m[1][0], m.m[1][1], m.m[1][2]).Length(),
			CalyxMath::Vector3(m.m[2][0], m.m[2][1], m.m[2][2]).Length()};
	}

	CalyxMath::Quaternion ExtractWorldRotation(const CalyxMath::Matrix4x4& m, const CalyxMath::Vector3& scale) {
		CalyxMath::Matrix4x4 rot = m;
		if(scale.x > 0.0001f) {
			rot.m[0][0] /= scale.x;
			rot.m[0][1] /= scale.x;
			rot.m[0][2] /= scale.x;
		}
		if(scale.y > 0.0001f) {
			rot.m[1][0] /= scale.y;
			rot.m[1][1] /= scale.y;
			rot.m[1][2] /= scale.y;
		}
		if(scale.z > 0.0001f) {
			rot.m[2][0] /= scale.z;
			rot.m[2][1] /= scale.z;
			rot.m[2][2] /= scale.z;
		}
		rot.m[3][0] = rot.m[3][1] = rot.m[3][2] = 0.0f;
		rot.m[3][3] = 1.0f;
		return CalyxMath::Quaternion::FromMatrix(rot);
	}
}

namespace CalyxEffect {

	/////////////////////////////////////////////////////////////////////////////////////////
	//		コンストラクタ/デストラクタ
	/////////////////////////////////////////////////////////////////////////////////////////
	ParticleSystemObject::ParticleSystemObject() {
		SceneObject::SetName("ParticleSystemObject", ObjectType::Effect);
		emitter_ = std::make_shared<CalyxEffect::FxEmitter>();

		std::cout << "[CTOR] FxObject GUID=" << GetGuid().ToString() << std::endl;
	}
	ParticleSystemObject::ParticleSystemObject(const std::string& name) {
		SceneObject::SetName(name, ObjectType::Effect);

		// エミッター
		emitter_ = std::make_shared<CalyxEffect::FxEmitter>();

		// デフォルト値の設定
		emitter_->velocity_.SetConstant({0.0f, 2.0f, 0.0f});
		emitter_->lifetime_.SetConstant({1.0f});
		emitter_->scale_.SetConstant({1.0f, 1.0f, 1.0f});

		std::cout << "[CTOR] FxObject GUID=" << GetGuid().ToString() << std::endl;
	}
	ParticleSystemObject::~ParticleSystemObject() = default;
	/////////////////////////////////////////////////////////////////////////////////////////
	//		常時更新
	/////////////////////////////////////////////////////////////////////////////////////////
	void ParticleSystemObject::AlwaysUpdate([[maybe_unused]] float dt) {
		// Gizmo操作中/停止中でも見た目と発生位置を一致させるため常時同期する
		worldTransform_.Update();
		emitter_->SetPosition(worldTransform_.GetWorldPosition());
		const auto worldScale = ExtractWorldScale(worldTransform_.matrix.world);
		emitter_->SetWorldScale(worldScale);
		emitter_->SetWorldRotation(ExtractWorldRotation(worldTransform_.matrix.world, worldScale));

		emitter_->Update(dt);
		emitter_->DrawEmitterShape(worldTransform_);
	}

	/////////////////////////////////////////////////////////////////////////////////////////
	//		debug gui
	/////////////////////////////////////////////////////////////////////////////////////////
	void ParticleSystemObject::ShowGui() {
		emitter_->ShowGui();
	}

	void ParticleSystemObject::SetDrawEnable(bool isDrawEnable) {
		emitter_->SetDrawEnable(isDrawEnable);
		// 子にも適用
		for(const auto& child : children_) {
			if(auto ps = std::dynamic_pointer_cast<ParticleSystemObject>(child)) {
				ps->SetDrawEnable(isDrawEnable);
			}
		}
	}

	void ParticleSystemObject::SetPosition(const CalyxMath::Vector3& pos) {
		emitter_->SetPosition(pos);
	}

	void ParticleSystemObject::ApplyConfig() {
		const auto& cfg = config_.GetConfig();

		// CalyxEffect::FxEmitter 設定反映
		emitter_->ApplyConfigFrom(cfg);
		// SceneObject 情報
		name_	  = cfg.name;
		parentId_ = cfg.parentGuid;

		worldTransform_.ApplyConfig(cfg.transform);
		worldTransform_.Update();
		emitter_->SetPosition(worldTransform_.GetWorldPosition());
		const auto worldScale = ExtractWorldScale(worldTransform_.matrix.world);
		emitter_->SetWorldScale(worldScale);
		emitter_->SetWorldRotation(ExtractWorldRotation(worldTransform_.matrix.world, worldScale));
	}

	void ParticleSystemObject::ExtractConfig() {
			auto& cfg = config_.GetConfig();
			emitter_->ExtractConfigTo(cfg); // config_ は ParticleSystemObjectConfig

			cfg.name	   = name_;
			cfg.parentGuid = parentId_;
			cfg.transform  = worldTransform_.ExtractConfig();
		}

	void ParticleSystemObject::ApplyConfigFromJson(const nlohmann::json& j) {
		config_.ApplyConfigFromJson(j);
		ApplyConfig();
	}

	void ParticleSystemObject::ExtractConfigToJson(nlohmann::json& j) const {
		const_cast<ParticleSystemObject*>(this)->ExtractConfig();
		config_.ExtractConfigToJson(j);
	}

	void ParticleSystemObject::LoadConfig(const std::string& path) {
		config_.LoadConfig(path);
		ApplyConfig();
	}

	void ParticleSystemObject::SaveConfig(const std::string& path) const {
		const_cast<ParticleSystemObject*>(this)->ExtractConfig();
		config_.SaveConfig(path);
	}

	void ParticleSystemObject::PlayRecursive() const {
		emitter_->Play();
		for(const auto& child : children_) {
			if(auto ps = std::dynamic_pointer_cast<ParticleSystemObject>(child)) {
				ps->PlayRecursive();
			}
		}
	}

	void ParticleSystemObject::StopRecursive() const {
		emitter_->Stop();
		for(const auto& child : children_) {
			if(auto ps = std::dynamic_pointer_cast<ParticleSystemObject>(child)) {
				ps->StopRecursive();
			}
		}
	}

	void ParticleSystemObject::ResetRecursive() const {
		emitter_->Reset();
		for(const auto& child : children_) {
			if(auto ps = std::dynamic_pointer_cast<ParticleSystemObject>(child)) {
				ps->ResetRecursive();
			}
		}
	}

	void ParticleSystemObject::Play() const { emitter_->Play(); }
	void ParticleSystemObject::Stop() const { emitter_->Stop(); }
	void ParticleSystemObject::Reset() const { emitter_->Reset(); }

	void ParticleSystemObject::SetAlphaMultiplier(float a) {
		if(emitter_) emitter_->SetAlphaMultiplier(a);
		for(auto& child : children_) {
			if(auto ps = std::dynamic_pointer_cast<ParticleSystemObject>(child)) {
				ps->SetAlphaMultiplier(a);
			}
		}
	}

	void ParticleSystemObject::SetCameraFade(float nearZ, float farZ) {
		if(emitter_) emitter_->SetCameraFade(nearZ, farZ);
		for(auto& child : children_) {
			if(auto ps = std::dynamic_pointer_cast<ParticleSystemObject>(child)) {
				ps->SetCameraFade(nearZ, farZ);
			}
		}
	}

} // namespace CalyxEffect