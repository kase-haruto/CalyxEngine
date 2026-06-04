#pragma once
#include <Engine/Scene/Context/SceneContext.h>
#include <Engine/System/Command/EditorCommand/BaseLevelEditorCommand.h>
#include <functional>
#include <memory>

template <class TObject>
class CreateObjectCommand final
	: public BaseLevelEditorCommand {
public:
	using Factory = std::function<std::shared_ptr<TObject>()>;

	CreateObjectCommand(SceneContext* ctx,
						Factory		  factory,
						const char*	  label = "CreateObject")
		: BaseLevelEditorCommand(label),
		  ctx_(ctx),
		  factory_(std::move(factory)) {}

	void Execute() override {
		object_ = factory_();
		// Note: SceneContext::Instantiate already adds the object to the library,
		// so we don't need to call ctx_->AddObject(object_) here.
		// Adding it twice causes crashes and ImGui assertions.
	}
	void Undo() override {
		ctx_->RemoveObject(object_);
		object_.reset();
	}

private:
	SceneContext*			 ctx_;
	Factory					 factory_;
	std::shared_ptr<TObject> object_;
};