#include "ScopedGizmoCommand.h"
#include <Engine/Objects/Transform/Transform.h>

#include <algorithm>



ScopedGizmoCommand::ScopedGizmoCommand(WorldTransform* transform, ImGuizmo::OPERATION op)
	: transform_(transform), op_(op){
	if(transform_) {
		transforms_.push_back(transform_);
		befores_.push_back(TransformSnapshot::FromTransform(transform_));
		before_ = befores_.front();
	}
}

ScopedGizmoCommand::ScopedGizmoCommand(const std::vector<WorldTransform*>& transforms, ImGuizmo::OPERATION op)
	: transform_(transforms.empty() ? nullptr : transforms.front()), transforms_(transforms), op_(op) {
	befores_.reserve(transforms_.size());
	for(auto* transform : transforms_) {
		if(transform) {
			befores_.push_back(TransformSnapshot::FromTransform(transform));
		}
	}
	if(!befores_.empty()) {
		before_ = befores_.front();
	}
}

void ScopedGizmoCommand::CaptureAfter(){
	afters_.clear();
	afters_.reserve(transforms_.size());
	for(auto* transform : transforms_) {
		if(transform) {
			afters_.push_back(TransformSnapshot::FromTransform(transform));
		}
	}
	if(!afters_.empty()) {
		after_ = afters_.front();
	}
	captured_ = true;
}

bool ScopedGizmoCommand::IsTrivial(float epsilon) const{
	if(!captured_ || befores_.size() != afters_.size()) return true;
	for(size_t i = 0; i < befores_.size(); ++i) {
		if(!befores_[i].Equals(afters_[i], epsilon)) {
			return false;
		}
	}
	return true;
}

void ScopedGizmoCommand::Execute(){
	if(!captured_) return;
	const size_t count = (std::min)(transforms_.size(), afters_.size());
	for(size_t i = 0; i < count; ++i) {
		if(transforms_[i]) afters_[i].ApplyToTransform(transforms_[i]);
	}
}

void ScopedGizmoCommand::Undo(){
	if(!captured_) return;
	const size_t count = (std::min)(transforms_.size(), befores_.size());
	for(size_t i = 0; i < count; ++i) {
		if(transforms_[i]) befores_[i].ApplyToTransform(transforms_[i]);
	}
}

const char* ScopedGizmoCommand::GetName() const{
	return name_.c_str();
}
