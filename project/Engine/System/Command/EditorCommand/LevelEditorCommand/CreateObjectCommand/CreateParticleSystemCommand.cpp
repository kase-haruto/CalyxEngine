#include "CreateParticleSystemCommand.h"

// engine
#include <Engine/Application/Effects/FxSystem.h>
#include <Engine/Application/Effects/Particle/Object/ParticleSystemObject.h>
#include <Engine/Scene/Context/SceneContext.h>

namespace CalyxEffect {
	CreateParticleSystemObjectCommand::CreateParticleSystemObjectCommand(
		SceneContext* context,
		ObjectFactory factory,
		std::string	  name)
		: context_(context),
		  factory_(std::move(factory)),
		  name_(std::move(name)) {}

	void CreateParticleSystemObjectCommand::Execute() {
		auto obj		= factory_();
		particleSystem_ = obj;

		// SceneObjectLibrary へ登録
		context_->GetObjectLibrary()->AddObject(obj);

		// FxSystem は参照だけ保持
		context_->GetFxSystem()->AddEmitter(
			particleSystem_->GetEmitter(),
			particleSystem_->GetGuid());
	}

	void CreateParticleSystemObjectCommand::Undo() {
		if(!particleSystem_) return;

		// サブシステムから先に外す
		context_->GetFxSystem()->RemoveEmitter(particleSystem_->GetEmitter().get());

		// ライブラリから削除
		context_->GetObjectLibrary()->RemoveObject(particleSystem_);

		particleSystem_.reset();
	}

	const char* CreateParticleSystemObjectCommand::GetName() const {
		return name_.empty() ? "" : name_.c_str();
	}
}
