#include "CreateShapeObjectCommand.h"
#include <Engine/Scene/Context/SceneContext.h>
#include <Engine/Scene/Utility/SceneUtility.h>

CreateShapeObjectCommand::CreateShapeObjectCommand(SceneContext* context,
												   ObjectFactory  factory)
	: context_(context),
	factory_(std::move(factory)){}

void CreateShapeObjectCommand::Execute(){
	object_ = factory_();

	// SceneObjectLibrary へ登録
	context_->GetObjectLibrary()->AddObject(object_);
}

void CreateShapeObjectCommand::Undo(){
	if (!object_) return;

	// ライブラリから削除
	context_->GetObjectLibrary()->RemoveObject(object_);

	object_.reset();
}

const char* CreateShapeObjectCommand::GetName() const{
	return name_.c_str();
}