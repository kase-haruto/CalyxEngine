#include "CreateShapeObjectCommand.h"
#include <Engine/Scene/Context/SceneContext.h>

CreateShapeObjectCommand::CreateShapeObjectCommand(SceneContext* context, ObjectFactory factory)
	: context_(context), factory_(std::move(factory)){}

void CreateShapeObjectCommand::Execute(){
	object_ = factory_();
	context_->AddEditorObject(object_);
}

void CreateShapeObjectCommand::Undo(){
	if (object_){
		context_->RemoveEditorObject(object_);
		object_.reset();
	}
}

const char* CreateShapeObjectCommand::GetName() const{
	return name_.c_str();
}
