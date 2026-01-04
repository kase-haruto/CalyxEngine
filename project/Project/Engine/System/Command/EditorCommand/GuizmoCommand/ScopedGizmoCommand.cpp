#include "ScopedGizmoCommand.h"
#include <Engine/Objects/Transform/Transform.h>



ScopedGizmoCommand::ScopedGizmoCommand(WorldTransform* transform, ImGuizmo::OPERATION op)
	: transform_(transform), op_(op){
	before_ = TransformSnapshot::FromTransform(transform_);
}

void ScopedGizmoCommand::CaptureAfter(){
	after_ = TransformSnapshot::FromTransform(transform_);
	captured_ = true;
}

bool ScopedGizmoCommand::IsTrivial(float epsilon) const{
	return !captured_ || before_.Equals(after_, epsilon);
}

void ScopedGizmoCommand::Execute(){
	if (captured_)after_.ApplyToTransform(transform_);
}

void ScopedGizmoCommand::Undo(){
	if (captured_)before_.ApplyToTransform(transform_);
}

const char* ScopedGizmoCommand::GetName() const{
	return name_.c_str();
}
