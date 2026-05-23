#pragma once

#include <Engine/Assets/Model/SplineDeformedModel.h>
#include <Engine/Foundation/Reflection/CalyxReflection.h>
#include <Engine/Objects/3D/Actor/BaseGameObject.h>
#include <Engine/Objects/3D/Geometory/Spline/SplineData.h>

#include <cstdint>
#include <string>

CALYX_OBJECT(Category = GameObject, DisplayName = "Spline Wall Deform", Icon = "UI/Tool/cylinder.dds")
class SplineDeformObject final : public BaseGameObject {
public:
	SplineDeformObject();
	~SplineDeformObject() override = default;

	void Initialize() override;
	void AlwaysUpdate(float dt) override;
	void DerivativeGui() override;

	std::string_view GetObjectClassName() const override { return "SplineDeformObject"; }
	void ApplyDerivedConfigFromJson(const nlohmann::json& root, const nlohmann::json* derived) override;
	void ExtractDerivedConfigToJson(nlohmann::json& root, nlohmann::json& derived) const override;

private:
	void EnsureDeformModel();
	void MarkDirty() { dirty_ = true; }
	bool Rebuild();

private:
	std::string sourceModelFile_ = "wall.obj";
	std::string splinePath_ = "Resources/Assets/Spline/Rail.json";
	int deformAxis_ = static_cast<int>(SplineDeformedModel::Axis::X);
	float radiusScale_ = 1.0f;
	float distanceOffset_ = 0.0f;
	bool autoRebuild_ = true;
	bool dirty_ = true;
	uint64_t lastSplineRevision_ = static_cast<uint64_t>(-1);
};
