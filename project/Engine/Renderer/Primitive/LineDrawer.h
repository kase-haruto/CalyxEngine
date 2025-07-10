#pragma once
#include <vector>
#include <Engine/Renderer/Mesh/VertexData.h>
#include <Engine/Graphics/Buffer/DxVertexBuffer.h>
#include <Engine/Graphics/Buffer/DxConstantBuffer.h>
#include <Engine/Objects/Transform/Transform.h>
#include <Engine/Graphics/Pipeline/PipelineDesc/Input/VertexLayout.h>

#include<vector>

struct Vector3;
struct Vector4;
struct Matrix4x4;

class LineDrawer{
public:
	void Initialize();
	void DrawLine(const Vector3& start, const Vector3& end, const Vector4& color);
	void Render();
	void Clear();

private:
	std::vector<VertexPosColor> vertices_;
	DxVertexBuffer<VertexPosColor> vertexBuffer_;
	DxConstantBuffer<TransformationMatrix> transformBuffer_;


private:
	static constexpr size_t kMaxLines = 1024; // 最大描画数
};
