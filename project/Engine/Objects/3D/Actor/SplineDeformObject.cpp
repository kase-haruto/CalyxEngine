#include "SplineDeformObject.h"

#include <Engine/Objects/3D/Actor/Registry/SceneObjectRegistry.h>
#include <Engine/Objects/3D/Geometory/Spline/SplineRegistry.h>

#include <externals/imgui/imgui.h>

#include <algorithm>
#include <array>

namespace {
	void CopyToBuffer(const std::string& value, std::array<char, 260>& buffer) {
		buffer.fill('\0');
		const size_t count = (std::min)(value.size(), buffer.size() - 1);
		std::copy_n(value.data(), count, buffer.data());
	}
}

SplineDeformObject::SplineDeformObject()
	: BaseGameObject() {
	SetName("SplineWallDeform");
	objectModelType_ = ObjectModelType::ModelType_Static;
	EnsureDeformModel();
}

void SplineDeformObject::Initialize() {
	EnsureDeformModel();
	MarkDirty();
}

void SplineDeformObject::EnsureDeformModel() {
	auto* deformModel = dynamic_cast<SplineDeformedModel*>(model_.get());
	if(deformModel) {
		deformModel->SetSourceModel(sourceModelFile_);
		return;
	}

	model_ = std::make_unique<SplineDeformedModel>(sourceModelFile_);
	objectModelType_ = ObjectModelType::ModelType_Static;
}

void SplineDeformObject::AlwaysUpdate(float dt) {
	EnsureDeformModel();
	if(auto spline = SplineRegistry::GetOrLoad(splinePath_)) {
		if(spline->Revision() != lastSplineRevision_) {
			MarkDirty();
		}
	}
	if(autoRebuild_ && dirty_) {
		Rebuild();
	}
	BaseGameObject::AlwaysUpdate(dt);
}

bool SplineDeformObject::Rebuild() {
	EnsureDeformModel();

	auto* deformModel = dynamic_cast<SplineDeformedModel*>(model_.get());
	if(!deformModel) {
		return false;
	}

	auto spline = SplineRegistry::GetOrLoad(splinePath_);
	if(!spline) {
		return false;
	}
	spline->BuildArcTable();

	const bool rebuilt = deformModel->Rebuild(
		*spline,
		static_cast<SplineDeformedModel::Axis>(deformAxis_),
		radiusScale_,
		distanceOffset_);
	if(rebuilt) {
		lastSplineRevision_ = spline->Revision();
		dirty_ = false;
	}
	return rebuilt;
}

void SplineDeformObject::DerivativeGui() {
	ImGui::SeparatorText("Spline Deform");

	std::array<char, 260> modelBuffer{};
	CopyToBuffer(sourceModelFile_, modelBuffer);
	if(ImGui::InputText("Source Model", modelBuffer.data(), modelBuffer.size())) {
		sourceModelFile_ = modelBuffer.data();
		EnsureDeformModel();
		MarkDirty();
	}

	std::array<char, 260> splineBuffer{};
	CopyToBuffer(splinePath_, splineBuffer);
	if(ImGui::InputText("Spline Path", splineBuffer.data(), splineBuffer.size())) {
		splinePath_ = splineBuffer.data();
		lastSplineRevision_ = static_cast<uint64_t>(-1);
		MarkDirty();
	}

	const char* axes[] = {"X", "Y", "Z"};
	if(ImGui::Combo("Deform Axis", &deformAxis_, axes, IM_ARRAYSIZE(axes))) {
		MarkDirty();
	}
	if(ImGui::DragFloat("Radius Scale", &radiusScale_, 0.01f, 0.01f, 100.0f)) {
		MarkDirty();
	}
	if(ImGui::DragFloat("Distance Offset", &distanceOffset_, 0.05f)) {
		MarkDirty();
	}
	if(ImGui::Checkbox("Auto Rebuild", &autoRebuild_)) {
		MarkDirty();
	}
	if(ImGui::Button("Rebuild Spline Mesh")) {
		Rebuild();
	}

	if(auto* deformModel = dynamic_cast<SplineDeformedModel*>(model_.get())) {
		ImGui::TextDisabled("Source: %s", deformModel->IsSourceReady() ? "ready" : "loading/not found");
	}
}

void SplineDeformObject::ApplyDerivedConfigFromJson(const nlohmann::json& root, const nlohmann::json* derived) {
	const nlohmann::json* src = derived ? derived : &root;
	sourceModelFile_ = src->value("sourceModelFile", sourceModelFile_);
	splinePath_ = src->value("splinePath", splinePath_);
	deformAxis_ = src->value("deformAxis", deformAxis_);
	radiusScale_ = src->value("radiusScale", radiusScale_);
	distanceOffset_ = src->value("distanceOffset", distanceOffset_);
	autoRebuild_ = src->value("autoRebuild", autoRebuild_);

	deformAxis_ = std::clamp(deformAxis_, 0, 2);
	lastSplineRevision_ = static_cast<uint64_t>(-1);
	EnsureDeformModel();
	MarkDirty();
}

void SplineDeformObject::ExtractDerivedConfigToJson(nlohmann::json&, nlohmann::json& derived) const {
	derived["sourceModelFile"] = sourceModelFile_;
	derived["splinePath"] = splinePath_;
	derived["deformAxis"] = deformAxis_;
	derived["radiusScale"] = radiusScale_;
	derived["distanceOffset"] = distanceOffset_;
	derived["autoRebuild"] = autoRebuild_;
}

REGISTER_SCENE_OBJECT(SplineDeformObject)
