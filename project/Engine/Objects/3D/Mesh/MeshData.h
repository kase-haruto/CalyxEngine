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
/**
 * @brief MeshDataに関するデータを保持する構造体です。
 */
struct MeshData {
	/**
	 * @brief SubMeshに関するデータを保持する構造体です。
	 */
	struct SubMesh {
		uint32_t indexStart = 0;
		uint32_t indexCount = 0;
		uint32_t materialIndex = 0;
	};

	std::vector<VertexPosUvN> vertices;
	std::vector<uint32_t>     indices;
	MaterialData              material;
	std::vector<MaterialData> materials;
	std::vector<SubMesh>      subMeshes;

	MaterialData&       Material() { return material; }
	const MaterialData& Material() const { return material; }

	std::vector<MaterialData>&       Materials() { return materials; }
	const std::vector<MaterialData>& Materials() const { return materials; }

	std::vector<SubMesh>&       SubMeshes() { return subMeshes; }
	const std::vector<SubMesh>& SubMeshes() const { return subMeshes; }

	std::vector<VertexPosUvN>&       Vertices() { return vertices; }
	const std::vector<VertexPosUvN>& Vertices() const { return vertices; }

	std::vector<uint32_t>&       Indices() { return indices; }
	const std::vector<uint32_t>& Indices() const { return indices; }
};

/*----------------------------------------------------------
 * MeshBuffers
 * - メッシュ用 GPU バッファ構造体
 *--------------------------------------------------------*/
/**
 * @brief MeshBuffersに関するデータを保持する構造体です。
 */
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
/**
 * @brief MeshResourceに関するデータを保持する構造体です。
 */
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

	std::vector<MaterialData>&       Materials() { return data.materials; }
	const std::vector<MaterialData>& Materials() const { return data.materials; }

	std::vector<MeshData::SubMesh>&       SubMeshes() { return data.subMeshes; }
	const std::vector<MeshData::SubMesh>& SubMeshes() const { return data.subMeshes; }

	std::vector<VertexPosUvN>&       Vertices() { return data.vertices; }
	const std::vector<VertexPosUvN>& Vertices() const { return data.vertices; }

	std::vector<uint32_t>&       Indices() { return data.indices; }
	const std::vector<uint32_t>& Indices() const { return data.indices; }

	DxVertexBuffer<VertexPosUvN>&       VertexBuffer() { return buffers.vertexBuffer; }
	const DxVertexBuffer<VertexPosUvN>& VertexBuffer() const { return buffers.vertexBuffer; }

	DxIndexBuffer<uint32_t>&       IndexBuffer() { return buffers.indexBuffer; }
	const DxIndexBuffer<uint32_t>& IndexBuffer() const { return buffers.indexBuffer; }
};
