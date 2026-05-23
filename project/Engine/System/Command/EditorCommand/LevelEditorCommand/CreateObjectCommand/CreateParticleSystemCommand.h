#pragma once

// engine
#include <Engine/System/Command/Interface/ICommand.h>

// c++
#include <functional>
#include <string>
#include <memory>

class SceneContext;

namespace CalyxEngine {
	class ParticleSystemObject;

	class CreateParticleSystemObjectCommand 
		: public ICommand {
	public:
		using ObjectFactory = std::function<std::shared_ptr<ParticleSystemObject>()>;

		CreateParticleSystemObjectCommand(SceneContext* context, ObjectFactory factory, std::string name);

		void		Execute() override;
		void		Undo() override;
		const char* GetName() const override;

	private:
		SceneContext*						  context_ = nullptr;
		ObjectFactory						  factory_;
		std::shared_ptr<ParticleSystemObject> particleSystem_; // 所有する
		std::string							  name_;
	};

}

