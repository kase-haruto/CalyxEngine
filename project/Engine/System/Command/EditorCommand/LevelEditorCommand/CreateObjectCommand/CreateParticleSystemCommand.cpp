#include "CreateParticleSystemCommand.h"

// engine
#include <Engine/Scene/Context/SceneContext.h>
#include <Engine/Application/Effects/FxSystem.h>
#include <Engine/Application/Effects/Particle/Object/ParticleSystemObject.h>

CreateParticleSystemObjectCommand::CreateParticleSystemObjectCommand(SceneContext* context, ObjectFactory factory, std::string name)
	: context_(context), factory_(std::move(factory)), name_(std::move(name)){}

void CreateParticleSystemObjectCommand::Execute(){
	auto obj = factory_();
	particleSystem_ = obj;

	// 所有権をEditorObjectとして登録
	context_->AddEditorObject<ParticleSystemObject>(std::move(obj));

	// FxSystemは参照だけ保持
	context_->GetFxSystem()->AddEmitter(particleSystem_);
}

void CreateParticleSystemObjectCommand::Undo(){
	if (particleSystem_){
		context_->GetFxSystem()->RemoveEmitter(particleSystem_);
		context_->RemoveEditorObject(particleSystem_); // ここでdeleteされる
		particleSystem_ = nullptr;
	}
}

const char* CreateParticleSystemObjectCommand::GetName() const{
	return name_.empty() ? "" : name_.c_str();
}
