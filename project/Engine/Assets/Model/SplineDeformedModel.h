#pragma once

#include "Model.h"

#include <Engine/Foundation/Math/Vector3.h>
#include <Engine/Objects/3D/Geometory/Spline/SplineData.h>

#include <memory>
#include <string>
#include <vector>

/**
 * @brief SplineDeformedModelの機能を提供するクラスです。
 */
class SplineDeformedModel final : public Model {
public:
	enum class Axis {
		X = 0,
		Y = 1,
		Z = 2,
	};

	SplineDeformedModel();
	explicit SplineDeformedModel(const std::string& sourceFile);
	~SplineDeformedModel() override = default;

	void Initialize() override;
	void Draw(const WorldTransform& transform) override;
	void ShowImGuiInterface() override;

	bool SetSourceModel(const std::string& sourceFile);
	bool Rebuild(const SplineData& spline, Axis axis, float radiusScale, float distanceOffset);
	bool IsSourceReady() const { return sourceReady_; }

private:
	void CreateMaterialBuffer() override;
	void MaterialBufferMap() override;
	void Map() override;

	bool LoadSourceIfReady();
	void UploadDeformedVertices();
	void RecalculateLocalAABB();

	static int AxisIndex(Axis axis);
	static int PerpIndexA(Axis axis);
	static int PerpIndexB(Axis axis);

private:
	std::unique_ptr<ModelData> ownedModelData_;
	std::vector<VertexPosUvN> originalVertices_;
	bool sourceReady_ = false;
	bool buffersReady_ = false;
};
