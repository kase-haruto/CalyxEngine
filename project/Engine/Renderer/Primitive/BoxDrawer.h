#pragma once
#include <vector>
#include <Engine/Renderer/Mesh/VertexData.h>
#include <Engine/Graphics/Buffer/DxVertexBuffer.h>
#include <Engine/Graphics/Buffer/DxConstantBuffer.h>
#include <Engine/Objects/Transform/Transform.h>
#include <Engine/Graphics/Pipeline/PipelineDesc/Input/VertexLayout.h>
struct CalyxEngine::Vector3;
struct CalyxEngine::Vector4;
struct Matrix4x4;

class BoxDrawer {
public:
	void Initialize();
	void DrawBox(const CalyxEngine::Vector3& center, const CalyxEngine::Quaternion& rotate, const CalyxEngine::Vector3& size, const CalyxEngine::Vector4& color);
	void Render();
	void Clear();

private:
	std::vector<VertexPosColor> vertices_;
	DxVertexBuffer<VertexPosColor> vertexBuffer_;
	DxConstantBuffer<TransformationMatrix> transformBuffer_;

private:
	static constexpr size_t kMaxBoxes = 256;
};
