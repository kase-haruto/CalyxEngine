#pragma once
#include <Engine/System/Command/EditorCommand/BaseLevelEditorCommand.h>
#include <Engine/Scene/Context/SceneContext.h>
#include <functional>
#include <memory>


template<class TObject>
class CreateObjectCommand final
	: public BaseLevelEditorCommand {
public:
	using Factory = std::function<std::shared_ptr<TObject>()>;

	CreateObjectCommand(SceneContext* ctx,
						Factory factory,
						const char* label = "CreateObject")
		: BaseLevelEditorCommand(label),
		ctx_(ctx),
		factory_(std::move(factory)) {}

  // ICommand 実装
	void Execute() override {
		object_ = factory_();
		//ctx_->AddObject(object_);
	}
	void Undo() override {
		ctx_->RemoveObject(object_);
		object_.reset();
	}

private:
	SceneContext* ctx_;
	Factory factory_;
	std::shared_ptr<TObject> object_;
};