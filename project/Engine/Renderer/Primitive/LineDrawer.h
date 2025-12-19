#pragma once
#include <vector>
#include <Engine/Renderer/Mesh/VertexData.h>
#include <Engine/Graphics/Buffer/DxVertexBuffer.h>
#include <Engine/Graphics/Buffer/DxConstantBuffer.h>
#include <Engine/Objects/Transform/Transform.h>
#include <Engine/Graphics/Pipeline/PipelineDesc/Input/VertexLayout.h>

#include<vector>

struct CalyxMath::Vector3;
struct CalyxMath::Vector4;
struct Matrix4x4;

class LineDrawer{
public:
	void Initialize();
	void DrawLine(const CalyxMath::Vector3& start, const CalyxMath::Vector3& end, const CalyxMath::Vector4& color);
	void Render();
	void Clear();

private:
	std::vector<VertexPosColor> vertices_;
	DxVertexBuffer<VertexPosColor> vertexBuffer_;
	DxConstantBuffer<TransformationMatrix> transformBuffer_;


private:
	static constexpr size_t kMaxLines = 6144; // 最大描画数
};
