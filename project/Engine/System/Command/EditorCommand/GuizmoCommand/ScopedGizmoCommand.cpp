#include "ScopedGizmoCommand.h"
#include <Engine/Objects/3D/Actor/Library/SceneObjectLibrary.h>
#include <Engine/Objects/3D/Actor/SceneObject.h>
#include <Engine/Objects/Transform/Transform.h>
#include <Engine/Scene/Context/SceneContext.h>

#include <algorithm>

namespace {
	std::string GetTransformTargetSummary(const std::vector<WorldTransform*>& transforms) {
		auto* context = SceneContext::Current();
		if(!context || !context->GetObjectLibrary()) return {};

		// Undo履歴を読みやすく保つため、対象名は最大3件まで表示する。
		std::string summary;
		std::size_t matchedCount = 0;
		for(const auto& object : context->GetObjectLibrary()->GetAllObjectsShared()) {
			if(!object) continue;
			if(std::find(transforms.begin(), transforms.end(), &object->GetWorldTransform()) == transforms.end()) continue;

			if(matchedCount < 3) {
				if(!summary.empty()) summary += ", ";
				summary += object->GetName();
			}
			++matchedCount;
		}
		if(matchedCount > 3) summary += ", ...";
		return summary;
	}

	std::string GetGizmoCommandName(ImGuizmo::OPERATION operation, const std::vector<WorldTransform*>& transforms) {
		const char* operationName = "Transform";
		if(operation & ImGuizmo::TRANSLATE) operationName = "Move";
		else if(operation & ImGuizmo::ROTATE) operationName = "Rotate";
		else if(operation & ImGuizmo::SCALE) operationName = "Scale";
		std::string name = std::string(operationName) + (transforms.size() > 1 ? " Objects" : " Object");
		const std::string targets = GetTransformTargetSummary(transforms);
		if(!targets.empty()) name += ": " + targets;
		return name;
	}
}


ScopedGizmoCommand::ScopedGizmoCommand(WorldTransform* transform, ImGuizmo::OPERATION op)
	: transform_(transform), op_(op){
	// 操作開始時点のTransformをSnapshot化し、Gizmo更新中の変更から独立させる。
	if(transform_) {
		transforms_.push_back(transform_);
		befores_.push_back(TransformSnapshot::FromTransform(transform_));
		before_ = befores_.front();
	}
	name_ = GetGizmoCommandName(op_, transforms_);
}

ScopedGizmoCommand::ScopedGizmoCommand(const std::vector<WorldTransform*>& transforms, ImGuizmo::OPERATION op)
	: transform_(transforms.empty() ? nullptr : transforms.front()), transforms_(transforms), op_(op) {
	// 複数選択時も入力順でSnapshotを保持し、各Transformとの対応を維持する。
	befores_.reserve(transforms_.size());
	for(auto* transform : transforms_) {
		if(transform) {
			befores_.push_back(TransformSnapshot::FromTransform(transform));
		}
	}
	if(!befores_.empty()) {
		before_ = befores_.front();
	}
	name_ = GetGizmoCommandName(op_, transforms_);
}

void ScopedGizmoCommand::CaptureAfter(){
	// Mouse Release時の最終状態を取得し、一連のDragを一つのCommandへまとめる。
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
	// 数値誤差範囲内で変化がない操作は履歴へ登録しない。
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
	// 破損した対応数でも範囲外参照しないよう、少ない側の件数だけ適用する。
	const size_t count = (std::min)(transforms_.size(), afters_.size());
	for(size_t i = 0; i < count; ++i) {
		if(transforms_[i]) afters_[i].ApplyToTransform(transforms_[i]);
	}
}

void ScopedGizmoCommand::Undo(){
	if(!captured_) return;
	// 操作開始時Snapshotを同じ対象順で復元し、親子Transformの個別値を戻す。
	const size_t count = (std::min)(transforms_.size(), befores_.size());
	for(size_t i = 0; i < count; ++i) {
		if(transforms_[i]) befores_[i].ApplyToTransform(transforms_[i]);
	}
}

const char* ScopedGizmoCommand::GetName() const{
	return name_.c_str();
}
