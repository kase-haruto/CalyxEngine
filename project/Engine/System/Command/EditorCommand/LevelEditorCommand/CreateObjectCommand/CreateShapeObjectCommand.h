#pragma once

#include <Engine/System/Command/Interface/ICommand.h>
#include <Engine/Objects/3D/Actor/BaseGameObject.h>

#include <memory>
#include <functional>

class SceneContext;


/**
 * @brief CreateShapeObjectCommandの機能を提供するクラスです。
 */
class CreateShapeObjectCommand
	: public ICommand{
public:
	using ObjectFactory = std::function<std::shared_ptr<BaseGameObject>()>;

	CreateShapeObjectCommand(SceneContext* context, ObjectFactory factory);

	void Execute() override;
	void Undo() override;
	const char* GetName() const override;

private:
	SceneContext* context_ = nullptr;
	ObjectFactory factory_;
	std::string name_;
	std::shared_ptr<BaseGameObject> object_;
};
