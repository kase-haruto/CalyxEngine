#include "ParticleSystemObject.h"

#include <Engine/Foundation/Clock/ClockManager.h>
#include <Engine/System/Command/EditorCommand/GuiCommand/ImGuiHelper/GuiCmd.h>
#include <Engine/Objects/3D/Actor/Registry/SceneObjectRegistry.h>
#include <Engine/Scene/Context/SceneContext.h>
#include <Engine/System/Event/EventBus.h>

ParticleSystemObject::ParticleSystemObject(const std::string& name){
	SceneObject::SetName(name, ObjectType::ParticleSystem);
}

ParticleSystemObject::~ParticleSystemObject(){
}

void ParticleSystemObject::Initialize(){
	
}

void ParticleSystemObject::Update(){
	// ワールド行列を更新
	worldTransform_.Update();
	position_ = worldTransform_.GetWorldPosition();

	// 自身（Emitter 部分）の更新
	FxEmitter::Update();

	// 子オブジェクトを再帰的に更新
	for (const auto& childSp : children_){
		if (auto ps = std::dynamic_pointer_cast< ParticleSystemObject >(childSp)){
			ps->Update();
		}
	}
}

void ParticleSystemObject::ShowGui(){
	config_.ShowGui();
	FxEmitter::ShowGui();

	// 子の GUI も展開
	/*for (const auto& child : children_){
		if (auto* ps = dynamic_cast< ParticleSystemObject* >(child)){
			ps->ShowGui();
		}
	}*/
}

void ParticleSystemObject::SetDrawEnable(bool isDrawEnable){
	FxEmitter::SetDrawEnable(isDrawEnable);

	// 子にも適用
	for (const auto& child : children_){
		if (auto ps = std::dynamic_pointer_cast< ParticleSystemObject >(child)){
			ps->SetDrawEnable(isDrawEnable);
		}
	}
}

void ParticleSystemObject::ApplyConfig(){
	const auto& cfg = config_.GetConfig();

	// FxEmitter 設定反映
	FxEmitter::ApplyConfigFrom(cfg); // config_ は ParticleSystemObjectConfig のはず

	// SceneObject 情報
	name_ = cfg.name;
	id_ = cfg.guid;
	parentId_ = cfg.parentGuid;

	worldTransform_.ApplyConfig(cfg.transform);
}

void ParticleSystemObject::ExtractConfig(){
	auto& cfg = config_.GetConfig();
	FxEmitter::ExtractConfigTo(cfg); // config_ は ParticleSystemObjectConfig

	cfg.name = name_;
	cfg.guid = id_;
	cfg.parentGuid = parentId_;
	worldTransform_.ExtractConfig();
}

void ParticleSystemObject::ApplyConfigFromJson(const nlohmann::json& j){
	config_.ApplyConfigFromJson(j);
	ApplyConfig();
}

void ParticleSystemObject::ExtractConfigToJson(nlohmann::json& j) const{
	const_cast< ParticleSystemObject* >(this)->ExtractConfig();
	config_.ExtractConfigToJson(j);
}

void ParticleSystemObject::PlayRecursive(){
	Play();
	for (const auto& child : children_){
		if (auto ps = std::dynamic_pointer_cast< ParticleSystemObject >(child)){
			ps->PlayRecursive();
		}
	}
}

void ParticleSystemObject::StopRecursive(){
	Stop();
	for (const auto& child : children_){
		if (auto ps = std::dynamic_pointer_cast< ParticleSystemObject >(child)){
			ps->StopRecursive();
		}
	}
}

void ParticleSystemObject::ResetRecursive(){
	Reset();
	for (const auto& child : children_){
		if (auto ps = std::dynamic_pointer_cast< ParticleSystemObject >(child)){
			ps->ResetRecursive();
		}
	}
}

REGISTER_SCENE_OBJECT(ParticleSystemObject)