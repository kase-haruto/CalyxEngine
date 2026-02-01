#pragma once

/* engine */
#include <Engine/Assets/Animation/AnimationStruct.h>
#include <Engine/Objects/3D/Geometory/AABB.h>
#include <Engine/Objects/3D/Mesh/MeshData.h>

/* c++ */
#include <map>

struct ModelData {
	ModelData() = default;
	ModelData(const ModelData&) = delete;
	ModelData& operator=(const ModelData&) = delete;
	ModelData(ModelData&&) noexcept = default;
	ModelData& operator=(ModelData&&) noexcept = default;

	MeshResource meshResource;
	AABB	 localAABB; // カリング判定用

	//-----------------------------------------------------------
	// アニメーション情報
	//-----------------------------------------------------------
	std::map<std::string, JointWeightData> skinClusterData;
	Animation							   animation;
	Skeleton							   skeleton;
	// std::vector<Animation> animations;
};

enum ObjectModelType {
	ModelType_Static,	 // 静的モデル
	ModelType_Animation, // アニメーションモデル
	ModelType_Unknown,	 // 不明
};