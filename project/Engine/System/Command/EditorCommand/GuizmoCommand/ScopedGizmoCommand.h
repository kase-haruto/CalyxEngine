#pragma once

#include <Engine/System/Command/EditorCommand/TransformCommand/TransformSnapshot.h>
#include <Engine/System/Command/Interface/ICommand.h>

#include <externals/imgui/imgui.h>
#include <externals/imgui/ImGuizmo.h>

#include <vector>

/* ========================================================================
/* ギズモコマンドラッパ
/* ===================================================================== */
/**
 * @brief ScopedGizmoCommandの機能を提供するクラスです。
 */
class ScopedGizmoCommand 
	: public ICommand {
public:
	ScopedGizmoCommand(WorldTransform* transform, ImGuizmo::OPERATION op);
	ScopedGizmoCommand(const std::vector<WorldTransform*>& transforms, ImGuizmo::OPERATION op);

	void CaptureAfter();
	bool IsTrivial(float epsilon = 1e-5f) const;

	void Execute() override;
	void Undo() override;
	const char* GetName() const override;

private:
	WorldTransform* transform_;
	std::vector<WorldTransform*> transforms_;
	ImGuizmo::OPERATION op_;
	TransformSnapshot before_;
	TransformSnapshot after_;
	std::vector<TransformSnapshot> befores_;
	std::vector<TransformSnapshot> afters_;
	bool captured_ = false;
	std::string name_;
};
