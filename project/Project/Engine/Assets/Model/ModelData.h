#pragma once

/* engine */
#include <Engine/Assets/Animation/AnimationStruct.h>
#include <Engine/Graphics/Buffer/DxIndexBuffer.h>
#include <Engine/Graphics/Buffer/DxVertexBuffer.h>
#include <Engine/Graphics/Material.h>
#include <Engine/Renderer/Mesh/VertexData.h>
#include <Engine/Graphics/Pipeline/PipelineDesc/Input/VertexLayout.h>
#include <Engine/Objects/3D/Mesh/MeshData.h>
#include <Engine/Objects/3D/Geometory/AABB.h>

/* c++ */
#include <d3d12.h>
#include <map>
#include <vector>
#include <wrl.h>

struct ModelData{
	MeshData meshData; // メッシュデータ
	AABB localAABB;		//カリング判定用

	//-----------------------------------------------------------
	// アニメーション情報
	//-----------------------------------------------------------
	std::map<std::string, JointWeightData> skinClusterData;
	Animation animation;
	Skeleton skeleton;
	// std::vector<Animation> animations;
	DxVertexBuffer<VertexPosUvN> vertexBuffer;
	DxIndexBuffer<uint32_t> indexBuffer;
};

enum ObjectModelType{
	ModelType_Static,		// 静的モデル
	ModelType_Animation,	// アニメーションモデル
	ModelType_Unknown,		// 不明
};
