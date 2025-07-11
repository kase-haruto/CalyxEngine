#pragma once
/* ========================================================================
/*	include space
/* ===================================================================== */
// engine
#include <Engine/Objects/Transform/Transform.h>
#include <Engine/Renderer/Primitive/PrimitiveDrawer.h>

// math
#include <Engine/Foundation/Math/Quaternion.h>
#include <Engine/Foundation/Math/Vector3.h>

// C++
#include <array>
#include <cstdint>
#include <map>
#include <optional>
#include <span>
#include <string>
#include <vector>

/////////////////////////////////////////////////////////////////////////////////////////
//          keyframe
/////////////////////////////////////////////////////////////////////////////////////////
template <typename tValue>
struct Keyframe{
	float time;     // アニメーション時間(秒)
	tValue value;   // 補間対象の値 (Vector3 or Quaternion)
};
using KeyframeQuaternion = Keyframe<Quaternion>;
using KeyframeVector3 = Keyframe<Vector3>;

/////////////////////////////////////////////////////////////////////////////////////////
//          AnimationCurve
/////////////////////////////////////////////////////////////////////////////////////////
template<typename tValue>
struct AnimationCurve{
	std::vector<Keyframe<tValue>> keyframes;
};

/////////////////////////////////////////////////////////////////////////////////////////
//          NodeAnimation
/////////////////////////////////////////////////////////////////////////////////////////
struct NodeAnimation{
	AnimationCurve<Vector3>     translate;	// 平行移動
	AnimationCurve<Quaternion>  rotate;		// 回転
	AnimationCurve<Vector3>     scale;		// スケーリング
};

struct Node{
	QuaternionTransform transform;
	Matrix4x4 localMatrix;
	std::string name;
	std::vector<Node> children;
};

/////////////////////////////////////////////////////////////////////////////////////////
//          Animation
/////////////////////////////////////////////////////////////////////////////////////////
struct Animation{
	float duration = 0.0f;  // アニメーション全体の尺(秒)
	std::map<std::string, NodeAnimation> nodeAnimations;
	std::vector<const NodeAnimation*> fastChannels;
};

/////////////////////////////////////////////////////////////////////////////////////////
//          skelton
/////////////////////////////////////////////////////////////////////////////////////////
struct Joint{
	QuaternionTransform restTransform;	//< バインドポーズ
	QuaternionTransform transform;	//< transform情報
	Matrix4x4 localMatrix;			//< ローカル行列
	Matrix4x4 skeletonSpaceMatrix;	//< スケルトン空間行列
	std::string name;				//< ボーン名
	std::vector<int32_t> children;	//< 子ボーンのインデックス
	int32_t index;					//< インデックス
	std::optional<int32_t> parent;	//< 親ボーンのインデックス
};

struct Skeleton{
	int32_t root;
	std::map<std::string, int32_t> jointMap;
	std::vector<Joint> joints;

	void JointDraw(const Matrix4x4& m, const Vector4& color);

	void Draw(const Matrix4x4& world = Matrix4x4::MakeIdentity(),
			  int highlightIndex = -1,
			  const Vector4& colorHi = { 1,0.2f,0.2f,1 });
};

/////////////////////////////////////////////////////////////////////////////////////////
//          skinning
/////////////////////////////////////////////////////////////////////////////////////////
struct VertexWeightData{
	float weight;
	uint32_t vertexIndex;
};

struct JointWeightData{
	Matrix4x4 inverseBindPoseMatrix;
	std::vector<VertexWeightData> vertexWeights;
};

/* ===============================================================
  influence
 =================================================================
頂点はweightの割合だけjointの影響を受ける。
頂点に対して影響を与える(受ける)パラメータ群のこと
============================================================== */
const uint32_t kNumMaxInfluence = 4; // 最大のジョイント影響数
struct VertexInfluence{
	std::array<float, kNumMaxInfluence> weights;			// ウェイト
	std::array<int32_t, kNumMaxInfluence> jointIndices;		// ジョイントインデックス
};

struct WellForGPU{
	Matrix4x4 skeletonSpaceMatrix;							//位置用
	Matrix4x4 skeletonSpaceInverseTransposeMatrix;			//法線用
};

struct SkinCluster{
	std::vector<Matrix4x4> inverseBindPoseMatrices;

	Microsoft::WRL::ComPtr<ID3D12Resource>influenceResource;
	D3D12_VERTEX_BUFFER_VIEW influenceBufferView;
	std::span<VertexInfluence>mappedInfluence;

	/* ===============================================================
		matrixPalette
	 =================================================================
	skinningを行う際に必要な行列をskeletonのjointの数だけ格納した配列のこと。
	paletteの色を置く１単位の場所をwellと呼ぶ。
	indexアクセスを必要とするためstructuedBufferを使用する。
	============================================================== */
	Microsoft::WRL::ComPtr<ID3D12Resource>paletteResource;
	std::span<WellForGPU>mappedPalette;
	std::pair<D3D12_CPU_DESCRIPTOR_HANDLE, D3D12_GPU_DESCRIPTOR_HANDLE> paletteSrvHandle;
};

struct AnimationState{
	std::string name;
	Animation animation;
	float currentTime = 0.0f;
	float speed = 1.0f;
	float weight = 1.0f;   // blending weight
	bool loop = true;
};