#pragma once

#include <Engine/Foundation/Math/Vector2.h>
#include <Engine/Foundation/Math/Vector3.h>
#include <Engine/Foundation/Math/Vector4.h>

#include <Engine/Renderer/Mesh/VertexData.h>

#include <d3d12.h>
#include <d3dx12.h>
#include <vector>

template<typename T>
struct VertexInputLayout;

/**
 * @brief VertexPosUvに関するデータを保持する構造体です。
 */
struct VertexPosUv {
	CalyxEngine::Vector3 pos;
	CalyxEngine::Vector2 uv;
};

/**
 * @brief VertexPosColorに関するデータを保持する構造体です。
 */
struct VertexPosColor {
	CalyxEngine::Vector3 pos;
	CalyxEngine::Vector4 color;
};

/**
 * @brief VertexPosUvColorに関するデータを保持する構造体です。
 */
struct VertexPosUvColor {
	CalyxEngine::Vector4 pos;
	CalyxEngine::Vector2 uv;
	CalyxEngine::Vector4 color;
};

/**
 * @brief VertexPosUvNに関するデータを保持する構造体です。
 */
struct VertexPosUvN {
	CalyxEngine::Vector4 position;	// 16 B
	CalyxEngine::Vector2 texcoord;	// 24 B
	CalyxEngine::Vector3 normal;		// 36 B
	CalyxEngine::Vector4 tangent = {1.0f, 0.0f, 0.0f, 1.0f};
};

/**
 * @brief VertexPosUvNSkinningに関するデータを保持する構造体です。
 */
struct VertexPosUvNSkinning {
	CalyxEngine::Vector4 pos;		// 16 B
	CalyxEngine::Vector2 uv;			// 24 B
	CalyxEngine::Vector3 normal;		// 36 B
	CalyxEngine::Vector4 tangent = {1.0f, 0.0f, 0.0f, 1.0f};
};

template<>
/**
 * @brief VertexInputLayoutに関するデータを保持する構造体です。
 */
struct VertexInputLayout<VertexPosUvN> {
	static std::vector<D3D12_INPUT_ELEMENT_DESC> Get() {
		return {
			// Semantic   Idx  Format                          Slot Offset
			{ "POSITION", 0,  DXGI_FORMAT_R32G32B32A32_FLOAT, 0,
			  D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },

			{ "TEXCOORD", 0,  DXGI_FORMAT_R32G32_FLOAT,       0,
			  D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },

			{ "NORMAL",   0,  DXGI_FORMAT_R32G32B32_FLOAT,    0,
			  D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },

			{ "TANGENT",  0,  DXGI_FORMAT_R32G32B32A32_FLOAT, 0,
			  D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		};
	}
};

template<>
/**
 * @brief VertexInputLayoutに関するデータを保持する構造体です。
 */
struct VertexInputLayout<VertexPosUv> {
	static std::vector<D3D12_INPUT_ELEMENT_DESC> Get() {
		return {
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0,
			  D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0,
			  D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		};
	}
};

template<>
/**
 * @brief VertexInputLayoutに関するデータを保持する構造体です。
 */
struct VertexInputLayout<VertexPosUvNSkinning>{
	static std::vector<D3D12_INPUT_ELEMENT_DESC> Get(){
		return {
			// Semantic   Idx  Format                          Slot Offset
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT,    0,  0,
			  D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },

			{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,          0, 16,
			  D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },

			{ "NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT,       0, 24,
			  D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },

			{ "TANGENT",  0, DXGI_FORMAT_R32G32B32A32_FLOAT,    0, 36,
			  D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },

			{ "WEIGHT",   0, DXGI_FORMAT_R32G32B32A32_FLOAT,    1,  0,
			  D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },

			{ "INDEX",    0, DXGI_FORMAT_R32G32B32A32_SINT,     1, 16,
			  D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		};
	}
};

template<>
/**
 * @brief VertexInputLayoutに関するデータを保持する構造体です。
 */
struct VertexInputLayout<VertexData> {
	static std::vector<D3D12_INPUT_ELEMENT_DESC> Get() {
		return {
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0,
			  0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },

			{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0,
			  sizeof(CalyxEngine::Vector4), D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
		};
	}
};
