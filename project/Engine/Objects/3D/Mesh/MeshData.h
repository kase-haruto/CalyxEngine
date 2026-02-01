#pragma once

#include "Engine/Graphics/Buffer/DxIndexBuffer.h"
#include "Engine/Graphics/Buffer/DxVertexBuffer.h"

#include <Engine/Graphics/Pipeline/PipelineDesc/Input/VertexLayout.h>
#include <Engine/Graphics/Material.h>

#include <vector>


/*----------------------------------------------------------
 * MeshData
 * - メッシュデータ構造体
 *--------------------------------------------------------*/
struct MeshData {
	std::vector<VertexPosUvN> vertices;
	std::vector<uint32_t>     indices;
	MaterialData              material;

	MaterialData&       Material() { return material; }
	const MaterialData& Material() const { return material; }

	std::vector<VertexPosUvN>&       Vertices() { return vertices; }
	const std::vector<VertexPosUvN>& Vertices() const { return vertices; }

	std::vector<uint32_t>&       Indices() { return indices; }
	const std::vector<uint32_t>& Indices() const { return indices; }
};

/*----------------------------------------------------------
 * MeshBuffers
 * - メッシュ用 GPU バッファ構造体
 *--------------------------------------------------------*/
struct MeshBuffers {
	DxVertexBuffer<VertexPosUvN> vertexBuffer;
	DxIndexBuffer<uint32_t>      indexBuffer;

	void SetCommand(ID3D12GraphicsCommandList* cmdList)const;

	DxVertexBuffer<VertexPosUvN>&       VertexBuffer() { return vertexBuffer; }
	const DxVertexBuffer<VertexPosUvN>& VertexBuffer() const { return vertexBuffer; }

	DxIndexBuffer<uint32_t>&       IndexBuffer() { return indexBuffer; }
	const DxIndexBuffer<uint32_t>& IndexBuffer() const { return indexBuffer; }
};

/*----------------------------------------------------------
 * MeshResource
 * - メッシュリソース構造体
 *--------------------------------------------------------*/
struct MeshResource {
	//===================================================================*/
	//			コピー禁止
	//===================================================================*/
	MeshResource() = default;
	MeshResource(const MeshResource&) = delete;
	MeshResource& operator=(const MeshResource&) = delete;

	MeshResource(MeshResource&&) = default;
	MeshResource& operator=(MeshResource&&) = default;

	//===================================================================*/
	//			public method
	//===================================================================*/
	MeshData    data;
	MeshBuffers buffers;
	D3D_PRIMITIVE_TOPOLOGY topology = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

	void SetCommand(ID3D12GraphicsCommandList* cmdList)const;

	MaterialData&       Material() { return data.material; }
	const MaterialData& Material() const { return data.material; }

	std::vector<VertexPosUvN>&       Vertices() { return data.vertices; }
	const std::vector<VertexPosUvN>& Vertices() const { return data.vertices; }

	std::vector<uint32_t>&       Indices() { return data.indices; }
	const std::vector<uint32_t>& Indices() const { return data.indices; }

	DxVertexBuffer<VertexPosUvN>&       VertexBuffer() { return buffers.vertexBuffer; }
	const DxVertexBuffer<VertexPosUvN>& VertexBuffer() const { return buffers.vertexBuffer; }

	DxIndexBuffer<uint32_t>&       IndexBuffer() { return buffers.indexBuffer; }
	const DxIndexBuffer<uint32_t>& IndexBuffer() const { return buffers.indexBuffer; }
};